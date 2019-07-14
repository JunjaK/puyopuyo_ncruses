#include <curses.h>
#include <cstdlib>
#include <ctime>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL.h>
#include <string>
using namespace std;

//ゲームのDelay parameterです。
#define GAME_DELAY 100000
#define LAND_DELAY 5000
#define VANISH_DELAY 300000000

const char* sounds[5] = { "bgm.wav", "landing.wav","switch.wav", "vanish.wav", "gameover.wav"};//効果音などの経路

// prototype for our audio callback`
// see the implementation for more information
void my_audio_callback(void *userdata, Uint8 *stream, int len);

// variable declarations
static Uint8 *audio_pos; // global pointer to the audio buffer to be played
static Uint32 audio_len; // remaining length of the sample we have to play

/*
** PLAYING A SOUND IS MUCH MORE COMPLICATED THAN IT SHOULD BE
*/
int PlayMusic(const char* MUS_PATH){

	// Initialize SDL.
	SDL_Init(SDL_INIT_AUDIO);

	// local variables
	static Uint32 wav_length; // length of our sample
	static Uint8 *wav_buffer; // buffer containing our audio file
	static SDL_AudioSpec wav_spec; // the specs of our piece of music
	
	
	/* Load the WAV */
	// the specs, length and buffer of our wav are filled
	SDL_LoadWAV(MUS_PATH, &wav_spec, &wav_buffer, &wav_length);
	// set the callback function
	wav_spec.callback = my_audio_callback;
	wav_spec.userdata = NULL;
	// set our global static variables
	audio_pos = wav_buffer; // copy sound buffer
	audio_len = wav_length; // copy file length
	
	/* Open the audio device */
	SDL_OpenAudio(&wav_spec, NULL);
	
	/* Start playing */
	SDL_PauseAudio(0);
    SDL_Delay(1);
	// shut everything down
	//SDL_FreeWAV(wav_buffer);
    //SDL_CloseAudio(); 

}

// audio callback function
// here you have to copy the data of your audio buffer into the
// requesting audio buffer (stream)
// you should only copy as much as the requested length (len)
void my_audio_callback(void *userdata, Uint8 *stream, int len) {
	
	if (audio_len ==0)
		return;
	
	len = ( len > audio_len ? audio_len : len );
	//SDL_memcpy (stream, audio_pos, len); 					// simply copy from one buffer into the other
	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);// mix from one buffer into another
	
	audio_pos += len;
	audio_len -= len;
}


//ぷよの色を示すenum
enum puyocolor {NONE,RED, BLUE, GREEN, YELLOW,BLOCK};

//ランダム数を通じてランダムな色を発生する関数。
int GetRandomNumber(int num){
	srand((unsigned int)time(NULL)+num);
	return rand()%4+1;
}

//ぷよぷよのゲームを全体的に管理するクラス。
class Puyo_Control
{
    private:
        int GameState; //ゲームのステイタスを示します。0ならGameStart画面、１ならプレイ画面、2ならゲームオーバを示します。
        int score; //　ぷよぷよの得点数
        int chain; 
        char** Screen; //Displayに使う2次元配列
        int ver,hor; //画面のsize

    public:
        //Constructorを使って初期化
        Puyo_Control(){
            GameState = 0;
            score = 0;
            chain = 0;
            ver = 0;
            hor = 0;
        }

        Puyo_Control(int x, int y){
            //ncurseのcolor settingです。
            init_pair(0, COLOR_WHITE, COLOR_BLACK);
            init_pair(1, COLOR_RED, COLOR_BLACK);
	        init_pair(2, COLOR_BLUE, COLOR_BLACK);
            init_pair(3, COLOR_GREEN, COLOR_BLACK);
            init_pair(4, COLOR_YELLOW, COLOR_BLACK);
            init_pair(5, COLOR_CYAN, COLOR_CYAN);
            init_pair(6, COLOR_WHITE, COLOR_WHITE);


            GameState = 0;
            ver = y+1;
            hor = x+1;
            score = 0;
            chain = 0;
            
            Screen = new char*[ver];
            for (int i = 0; i < ver; i++){
                Screen[i] = new char[hor];
                for (int j = 0; j < hor; j++){
                    Screen[i][j] = 0;
                }
            }
        }
        //Desturctorを使ってポインターのメモリ解除
        ~Puyo_Control(){
            for (int i = 0; i < ver; i++)
            {
                delete Screen[i];
            }
            delete[] Screen;
        }
        void Reset(){
            for (int i = 0; i < ver; i++)
            {
                delete Screen[i];
            }
            delete[] Screen;

            GameState = 0;
            score = 0;
            chain = 0;
            
            Screen = new char*[ver];
            for (int i = 0; i < ver; i++){
                Screen[i] = new char[hor];
                for (int j = 0; j < hor; j++){
                    Screen[i][j] = 0;
                }
            }
        }
        int GetScore(){
            return score;
        }
        char** GetScreen(){
            return Screen;
        }

        int GetGameState(){
            return GameState;
        }
        void SetGamestate(int n){
            GameState = n;
        }
        //ゲームのスターと画面です。
        void Display_first(){
            char msg[256];
            attrset(COLOR_PAIR(0));
            sprintf(msg, "==========================");
            mvaddstr(2, 2, msg);

            sprintf(msg, "===");
            mvaddstr(3, 2, msg);
            attrset(COLOR_PAIR(1));
            sprintf(msg, "Pu");
            mvaddstr(3, 11, msg);
            attrset(COLOR_PAIR(2));
            sprintf(msg, "yo");
            mvaddstr(3, 13, msg);
            attrset(COLOR_PAIR(3));
            sprintf(msg, "Pu");
            mvaddstr(3, 15, msg);
            attrset(COLOR_PAIR(4));
            sprintf(msg, "yo");
            mvaddstr(3, 17, msg);
            attrset(COLOR_PAIR(0));
            sprintf(msg, "      ===");
            mvaddstr(3, 19, msg);

            sprintf(msg, "===                    ===");
            mvaddstr(4, 2, msg);

            sprintf(msg, "=== Press 'S' to Start ===");
            mvaddstr(5, 2, msg);

            sprintf(msg, "=== Press 'Q' to Quit! ===");
            mvaddstr(6, 2, msg);

            sprintf(msg, "==========================");
            mvaddstr(7, 2, msg);

            refresh();        
        }

        //プレイ中の画面です。
        void Display_playing()
        {
            int j,i;
            for (i = 2; i < ver; i++)
            {
                //境界を作るためぷよを貯まる配列を0ではなく1から始めます。
                attrset(COLOR_PAIR(6));
                mvaddch(i, 0, '.');
                for (j = 1; j < hor-1 ; j++)
                {
                    switch (Screen[i][j])
                    {
                    case NONE:
                        attrset(COLOR_PAIR(0));
                        mvaddch(i, j, ' ');
                        break;
                    case RED:
                        attrset(COLOR_PAIR(1));
                        mvaddch(i, j, 'R');
                        break;
                    case BLUE:
                        attrset(COLOR_PAIR(2));
                        mvaddch(i, j, 'B');
                        break;
                    case GREEN:
                        attrset(COLOR_PAIR(3));
                        mvaddch(i, j, 'G');
                        break;
                    case YELLOW:
                        attrset(COLOR_PAIR(4));
                        mvaddch(i, j, 'Y');
                        break;
                    case BLOCK: //障害物
                        attrset(COLOR_PAIR(5));
                        mvaddch(i, j, ' ');
                        break;                        
                    default:
                        mvaddch(i, j, '?');
                        break;
                    }
                }
                attrset(COLOR_PAIR(6));
                mvaddch(i, j, '.');
            }
            attrset(COLOR_PAIR(0));
            for (j = 0; j < hor; j++){
                mvaddch(i,j, '-');
            }
            
            //ゲームの状態を表示します。
            char msg[256];
            sprintf(msg, "Field: %d x %d", ver - 2, hor - 2);

            attrset(COLOR_PAIR(0));
            mvaddstr(2, 20, msg);

            sprintf(msg, "Scores : %d", score);
            mvaddstr(3, 20, msg);

            sprintf(msg, "Max Chains : %d", chain);
            mvaddstr(4, 20, msg);

            sprintf(msg, "Restart Game : 'R'");
            mvaddstr(8, 20, msg);
            
            sprintf(msg, "Exit Game : 'Q'");
            mvaddstr(9, 20, msg);

            refresh();
        }

        //ゲームオーバの画面です。
        void Display_Over(){
            char msg[256];
            attrset(COLOR_PAIR(0));
            sprintf(msg, "==========================");
            mvaddstr(2, 2, msg);

            sprintf(msg, "===      GameOVer      ===");
            mvaddstr(3, 2, msg);

            sprintf(msg, "===  Your Score : %d",score);
            mvaddstr(4, 2, msg);

            sprintf(msg, "===  Your Chain : %d",chain);
            mvaddstr(5, 2, msg);

            sprintf(msg, "===  Press 'R' to Restart ");
            mvaddstr(6, 2, msg);

            sprintf(msg, "===  Press 'Q' to Quit!");
            mvaddstr(7, 2, msg);
 
            sprintf(msg, "==========================");
            mvaddstr(8, 2, msg);

            refresh();        
        }

        //状況によるにぷよぷよのゲーム画面を表します。
        void Display(int& a){
            if(GameState == 0){
                Display_first();
            }
            else if(GameState == 1){
                Display_playing();
            }
            else if(GameState == 2){
                if(a == 0){ //ゲーム終了音１回だけ実行する仕組み
                    PlayMusic(sounds[4]);
                    a++;
                }

                Display_Over();
            }
        }
        //ゲームの状態をCheckします。Screen配列の一番上に何かたまったらゲームオーバになります。
        void Check_State(){
            for (int i = 0; i < hor; i++){
                if(Screen[2][i] != 0 && Screen[3][i] != 0 && Screen[4][i] != 0){
                    GameState = 2;
                    clear();
                }
            }
        }
        void SetChain(int num){
            chain = num;
        }
        int GetChain(){
            return chain;
        }
        void SetScore(int num){
            score += num;
        }
};


//ぷよの形と動きを管理するPUYO暮らすです。ぷよを動くときはScreenに以前のぷよの位置を削除して新しい位置をScreenに貯蔵します。
class Puyo
{
private:
    //ぷよの横、縦の状態を管理する配列です。
    int shape_hor[1][2];
    int shape_ver[2][1];
    bool puyostate;         //true - horizon, false - vertical
    int screen_x, screen_y; //画面のsize
    int Now_x, Now_y;       //現在のぷよの位置を示します。
    int temp_now_y[2];
public:
    Puyo()
    {
        shape_hor[0][0] = 0;
        shape_hor[0][1] = 0;
        shape_ver[0][0] = 0;
        shape_ver[1][0] = 0;
        screen_x = 0;
        screen_y = 0;
        puyostate = true; //最初のぷよは横ならぶように設定します。
    }
    int Get_Screen_x() const
    {
        return screen_x;
    }
    int Get_Screen_y() const
    {
        return screen_y;
    }
    bool Get_Puyostate() const
    {
        return puyostate;
    }
    int Get_Now_x() const
    {
        return Now_x;
    }
    int Get_Now_y() const
    {
        return Now_y;
    }
    int* Get_temp_now_y(){
        return temp_now_y;
    }

    //ぷよの色を示すL, R、そして画面のsize x, yをもらいます。
    void Generate_Puyo(int L, int R, int x, int y)
    {
        shape_hor[0][0] = L;
        shape_hor[0][1] = R;
        screen_x = x;
        screen_y = y;
        Now_x = x / 2 - 1;
        Now_y = 1;
        puyostate = true;
    }

    //ぷよを左側に動く関数です。
    //Puyo_Controlで管理するScreen配列を入力因子として貰い落下中のぷよの現在位置から左側に1ドットくらい動きます。
    void MoveLeft(char *Sc[])
    {
        if (Now_x > 1 && Sc[Now_y][Now_x - 1] == 0)
        {
            //ぷよが横で長い状態の場合
            if (puyostate == true)
            {
                Sc[Now_y][Now_x - 1] = Sc[Now_y][Now_x];
                Sc[Now_y][Now_x] = Sc[Now_y][Now_x + 1];
                Sc[Now_y][Now_x + 1] = 0;
            }
            //ぷよが縦に長い状態の場合
            else if (puyostate == false)
            {
                Sc[Now_y][Now_x - 1] = Sc[Now_y][Now_x];
                Sc[Now_y - 1][Now_x - 1] = Sc[Now_y - 1][Now_x];
                Sc[Now_y][Now_x] = 0;
                Sc[Now_y - 1][Now_x] = 0;
            }
            //左側に動いたから現在x座標をマイナス１する必要があります。
            Now_x--;
        }
    }
    //ぷよを右側に動く関数です。
    //動く方式はMoveLeft関数と童謡で方向だけ違います。
    void MoveRight(char *Sc[])
    {
        if (puyostate == true){
            if (Now_x < screen_x - 2 && Sc[Now_y][Now_x + 2] == 0)
            {
                Sc[Now_y][Now_x + 2] = Sc[Now_y][Now_x + 1];
                Sc[Now_y][Now_x + 1] = Sc[Now_y][Now_x];
                Sc[Now_y][Now_x] = 0;
                Now_x++;
            }
        }
        else if (puyostate == false)
        {
            if (Now_x < screen_x - 1 && Sc[Now_y][Now_x + 1] == 0)
            {
                Sc[Now_y][Now_x + 1] = Sc[Now_y][Now_x];
                Sc[Now_y - 1][Now_x + 1] = Sc[Now_y - 1][Now_x];
                Sc[Now_y][Now_x] = 0;
                Sc[Now_y - 1][Now_x] = 0;
                Now_x++;
            }
        }
    }
    //ぷよを下方向Keyを押した場合さらに下に動く関数です。
    void MoveDown(char *Sc[])
    {
        //Landing判定ではなければ下の方向にどんどんいきます。
        if (!(Landing(Sc)))
            falling(Sc);
    }
    //ぷよを回転する関数です。ぷよの横、縦状態を貯めるshape_hor, shape_ver変数を利用します。
    void Rotate(char *Sc[])
    {
        if (puyostate == true)
        {
            if (Sc[Now_y + 1][Now_x] == 0)
            {
                puyostate = false;
                shape_ver[0][0] = shape_hor[0][0];
                shape_ver[1][0] = shape_hor[0][1];
                Sc[Now_y][Now_x + 1] = 0;
            }
        }
        else if (puyostate == false)
        {
            if (Sc[Now_y + 1][Now_x + 1] == 0)
            {
                puyostate = true;
                shape_hor[0][0] = shape_ver[1][0];
                shape_hor[0][1] = shape_ver[0][0];
                Sc[Now_y - 1][Now_x] = 0;
            }
        }
    }

    //ぷよの着地判定です。ぷよの現在状態(横か、館か)によって判断基準が違います。
    bool Landing(char *Sc[])
    {
        temp_now_y[0] = Now_y;
        temp_now_y[1] = Now_y;
        //ぷよがFloorまで到達する場合
        if (Now_y == screen_y)
            return true;
        //縦に長いぷよが落下の間貯まっていたぷよと会う場合
        if (puyostate == false)
        {
            if (Sc[Now_y + 1][Now_x] != 0)
                return true;
            else
                return false;
        }
        //縦に長いぷよが落下の間貯まっていたぷよと会う場合
        else if (puyostate == true)
        {
            //ぷよの接触面が２つある場合
            if (Sc[Now_y + 1][Now_x] != 0 && Sc[Now_y + 1][Now_x + 1] != 0)
                return true;
            //ぷよの接触面が1つしかない場合
            else if (Sc[Now_y + 1][Now_x] != 0 && Sc[Now_y + 1][Now_x + 1] == 0) //ぷよの右の法に接触面がない場合
            {
                int landDelay = 0;
                while (1)
                {
                    if (landDelay % LAND_DELAY == 0)
                    {
                        if (Now_y == screen_y)
                        {
                            Sc[Now_y][Now_x + 1] = shape_hor[0][1];
                            Sc[Now_y - 1][Now_x + 1] = 0;
                            break;
                        }
                        else if (Sc[Now_y + 1][Now_x + 1] == 0)
                        {
                            Now_y++;
                            Sc[Now_y][Now_x + 1] = shape_hor[0][1];
                            Sc[Now_y - 1][Now_x + 1] = 0;
                        }
                        else if (Sc[Now_y + 1][Now_x + 1] != 0)
                        {
                            Sc[Now_y][Now_x + 1] = shape_hor[0][1];
                            Sc[Now_y - 1][Now_x + 1] = 0;
                            break;
                        }
                    }
                    landDelay++;
                }
                temp_now_y[1] = 0;
                return true;
            }

            else if (Sc[Now_y + 1][Now_x] == 0 && Sc[Now_y + 1][Now_x + 1] != 0) //ぷよの左の法に接触面がない場合
            {
                int landDelay = 0;
                while (1)
                {
                    if (landDelay % 5000 == 0)
                    {
                        if (Now_y == screen_y)
                        {
                            Sc[Now_y][Now_x] = shape_hor[0][0];
                            Sc[Now_y - 1][Now_x] = 0;
                            break;
                        }
                        else if (Sc[Now_y + 1][Now_x] == 0)
                        {
                            Now_y++;
                            Sc[Now_y][Now_x] = shape_hor[0][0];
                            Sc[Now_y - 1][Now_x] = 0;
                        }
                        else if (Sc[Now_y + 1][Now_x] != 0)
                        {
                            Sc[Now_y][Now_x] = shape_hor[0][0];
                            Sc[Now_y - 1][Now_x] = 0;
                            break;
                        }
                    }
                    landDelay++;
                }
                temp_now_y[0] = 0; 
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    //落下の判定ですこれもぷよの状態によって違います。
    void falling(char *Sc[])
    {
        if (!Landing(Sc))
        {
            Now_y++;
            if (puyostate == true)
            {
                Sc[Now_y][Now_x] = shape_hor[0][0];
                Sc[Now_y][Now_x + 1] = shape_hor[0][1];
                Sc[Now_y - 1][Now_x] = 0;
                Sc[Now_y - 1][Now_x + 1] = 0;
            }
            else if (puyostate == false)
            {
                Sc[Now_y][Now_x] = shape_ver[1][0];
                Sc[Now_y - 1][Now_x] = shape_ver[0][0];
                if (Now_y - 2 >= 0)
                    Sc[Now_y - 2][Now_x] = 0;
            }
        }
    }
};

//同じ色のぷよが4個以上接触する場合削除する機能を追加するためにPuyoクラスを継承して機能を追加します。
class Puyo_Movement_Vanish : public Puyo
{
private:
    int near_puyo;  //同じ色のぷよがいくつあるのかを示す変数。
    char** near_screen; //
    int num_chain;
public:
    Puyo_Movement_Vanish() : Puyo(){
        near_puyo = 0;
        near_screen = NULL;
        num_chain = 0;
    }
    ~Puyo_Movement_Vanish(){
        for (int i = 0; i < Get_Screen_y(); i++)
        {
            delete near_screen[i];
        }
        delete[] near_screen;
    }

    void Generate_Puyo(int L, int R, int x, int y){
        //既にnear_screenが作られた場合メモリを解除する必要があります。                  
        Puyo::Generate_Puyo(L,R,x,y);
    }
    //ぷよを一番下まで動く関数です。
    void MoveFloor(char *Sc[]){
        while (!(Landing(Sc))){
            falling(Sc);
        }
    }

    //puyo_vanish関連配列メモリ解除
    void Release_NearPuyo(){
        if(near_screen!=NULL){
            for (int i = 0; i < Get_Screen_y()+1; i++){
                delete near_screen[i];
            }
            delete[] near_screen;
        }
        
    }
    // 障害物を消す仕組み。横に1列詰まったら消します。障害物を消した場合50点獲得です。
    int vanish_block(char* Sc[], Puyo_Control& puyopuyo){
        int score = 0;
        int count;
        char msg[256];
        for (int y = 2 ; y < Get_Screen_y()+1; y++){
            count = 0;
            for (int x = 1; x < Get_Screen_x(); x++){
                if (Sc[y][x] == 5)
                    count++;
            }
            if (count == Get_Screen_x() -1){
                clear();
                for (int i = 1; i < Get_Screen_x(); i++){
                    for (int c = y; c > 2; c--){
                        Sc[c][i] = Sc[c - 1][i];
                        if (Sc[c - 1][i] == 0)
                            break;
                    }
                }
                score += 50;
                sprintf(msg, "===Block Remove!===");
                mvaddstr(6, 20, msg);
                puyopuyo.Display(count);
                PlayMusic(sounds[3]); //vanishするときeffect3番再生 
            }
        }
        return score;
    }
    //puyo_vanishに必要な配列など初期化
    void initNearPuyo()
    {
        Release_NearPuyo();
        near_puyo= 0;
        near_screen = new char *[Get_Screen_y()+1];
        for (int i = 0; i < Get_Screen_y()+1; i++){
            near_screen[i] = new char[Get_Screen_x()];
            for (int j = 1; j < Get_Screen_x(); j++){
                near_screen[i][j] = 0;
            }
        }
    }

    //削除するぷよを計算するかんすうです。
    int countPuyo(char *Sc[], int y, int x, int color){
        if (x >= 1 && x < Get_Screen_x() && y >= 2 && y < Get_Screen_y() + 1){
            if (Sc[y][x] == color && near_screen[y][x] == 0){
                near_puyo++;
                near_screen[y][x] = 1; 
                countPuyo(Sc, y + 1, x, color);
                countPuyo(Sc, y, x + 1, color);
                countPuyo(Sc, y - 1, x, color);
                countPuyo(Sc, y, x - 1, color);
            }
        }
        else{
            return near_puyo;
        }
    }
    //削除するぷよの数が４を越える場合削除する関数です。
    int Destroy(char* Sc[]){
        if (near_puyo >= 4){
            for (int y = 2; y < Get_Screen_y()+1; y++){
                for (int x = 1; x < Get_Screen_x(); x++){
                    if(near_screen[y][x] == 1){
                        near_screen[y][x] = 0;
                        for (int c = y; c > 2; c--){
                            Sc[c][x] = Sc[c-1][x];
                            if(Sc[c-1][x] == 0)
                                break;
                        }
                    }
                }
            }
            num_chain++;
            return num_chain*near_puyo; //Score計算する方式です：chain * 近所のぷよ数
        }
        return 0;
    }

    //すべてのScreenをCheckしてcountPuyoの関数を通じて削除するぷよを得てDestroyを通じて削除する（条件満足し）
    int Vanishing(char *Sc[], Puyo_Control& puyopuyo){
        attrset(COLOR_PAIR(0));

        char msg[256];
        int temp;
        int score = 0;
        int check = 0;
        int vanish_delay = 0;
        num_chain = 0;
        for (int y = 2; y < Get_Screen_y() + 1; y++){
            for (int x = 1; x < Get_Screen_x(); x++){
                for (int i = 1; i < 5; i++){
                    if (Sc[y][x] != 0 && Sc[y][x] != 5){
                        initNearPuyo();
                        countPuyo(Sc, y, x, i);
                        check = Destroy(Sc); //VanishするぷよをつうじてScore計算
                        score += check;
                        if(check != 0){
                            for (int delay = 0; delay < VANISH_DELAY; delay++){
                                //dealyです
                            }
                            sprintf(msg, "---Now %d Combo!---", num_chain); //現在のCombo表示
                            mvaddstr(6, 20, msg);
                            puyopuyo.Display(temp);
                            PlayMusic(sounds[3]); //vanishするときeffect3番再生

                        }
                        check = 0;
                    }
                }
            }
        }
        return score;
    }
    int Get_Chain(){
        return num_chain;
    }
};

int main(){
//画面の初期化
	initscr();
	//カラー属性を扱うための初期化
	start_color();
	//キーを押しても画面に表示しない
	noecho();
	//キー入力を即座に受け付ける
	cbreak();

	curs_set(0);
	//キー入力受付方法指定
	keypad(stdscr, TRUE);

	//キー入力非ブロッキングモード
	timeout(0);
    
    int delay = 0;
    int score = 0;

    int L_Color, R_Color;

    //画面のsizeを指定する変数
    const int horizontal = 8;
    const int vertical = 12;
    int generate_count = 1;
    int input_key;
    int put_gameSound = 0;


    //puyopuyoの初期設定
    Puyo_Control puyopuyo(horizontal+1,vertical+1);

    L_Color = GetRandomNumber(1);
    R_Color = GetRandomNumber(6);

    Puyo_Movement_Vanish puyo;
    puyo.Generate_Puyo(L_Color, R_Color,horizontal+1,vertical+1);
    
    while (1)
    {
        input_key = getch();
        //ゲームの終了
        if (input_key == 'Q')
            break;
        //ゲームリスタート
        if (input_key == 'R'){
            clear();
            generate_count = 1;
            puyopuyo.Reset();
            puyo.Generate_Puyo(L_Color, R_Color, horizontal+1, vertical + 1);
            puyopuyo.SetGamestate(0);
        }
        //ゲームスターと
        if (input_key == 'S'){
            clear();
            puyopuyo.SetGamestate(1);
        }
        
        //現在ゲーム状態出力
        puyopuyo.Display(put_gameSound);
        //ぷよぷよゲーム状態が1(PLaying)ならゲームとして機能します。
        if(puyopuyo.GetGameState() == 1){

            delay++;
            //ぷよの動き
            switch (input_key)
            {
            case KEY_LEFT:
                PlayMusic(sounds[2]); //ぷよの操作するときeffect2番再生
                puyo.MoveLeft(puyopuyo.GetScreen());
                break;
            case KEY_RIGHT:
                PlayMusic(sounds[2]);//ぷよの操作するときeffect2番再生
                puyo.MoveRight(puyopuyo.GetScreen());
                break;
            case KEY_DOWN:
                PlayMusic(sounds[2]);//ぷよの操作するときeffect2番再生
                puyo.MoveDown(puyopuyo.GetScreen());
                break;
            case KEY_UP:
                PlayMusic(sounds[2]);//ぷよの操作するときeffect2番再生
                puyo.Rotate(puyopuyo.GetScreen());
                break;
            case ' ':
                PlayMusic(sounds[2]);//ぷよの操作するときeffect2番再生
                puyo.MoveFloor(puyopuyo.GetScreen());
                break;
            default:
                break;
            }

            if (delay % GAME_DELAY == 0)
            {
                clear();
                //ぷよがLanding判定場合ぷよを生成
                if (puyo.Landing(puyopuyo.GetScreen()))
                {
                    mvaddstr(6, 20, "                           ");
                    puyopuyo.Display(put_gameSound);
                    PlayMusic(sounds[1]); //ぷよが落下するときeffect１番再生
                    //VanishするぷよがあるならVanishします。
                    score = puyo.Vanishing(puyopuyo.GetScreen(), puyopuyo);
                    score += puyo.vanish_block(puyopuyo.GetScreen(), puyopuyo);

                    L_Color = GetRandomNumber(1);
                    R_Color = GetRandomNumber(6233);

                    //障害物生成ぷよが5回め生成の時は左だけ、15回目生成の時は左と、右全部生成します。
                    if(generate_count % 5 == 0){
                        L_Color = 5;
                    }
                    if(generate_count % 10 == 0){
                        R_Color = 5;
                    }

                    //点数計算表示
                    puyopuyo.SetScore(score);
                    //最大Chain数表示
                    if (puyopuyo.GetChain() < puyo.Get_Chain())
                        puyopuyo.SetChain(puyo.Get_Chain());

                    generate_count++; //障害物生成count
                    puyo.Generate_Puyo(L_Color, R_Color, horizontal+1, vertical + 1);

                    puyopuyo.Display(put_gameSound);
                }
                //Landing判定ではない場合ぷよを落下します。
                else
                {
                    puyo.falling(puyopuyo.GetScreen());
                }
                //ぷよぷよのゲームがOver状態かないか確認
                puyopuyo.Check_State();
            }
        }
    }

    endwin();
    return 0;
}