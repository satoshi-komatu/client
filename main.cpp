#include <Novice.h>
#include <math.h>
#include <process.h>
#include <mmsystem.h>
#include <fstream>

#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "winmm.lib")

using namespace std;

DWORD WINAPI Threadfunc(void*);

struct POS
{
	int x;
	int y;
};
POS pos1P, pos2P, old_pos2P;
RECT rect;
SOCKET sWait;
bool bSocket = false;
HWND hwMain;

const char kWindowTitle[] = "KAMATA ENGINEクライアント";

typedef struct {
	float x;
	float y;
}Vector2;

typedef struct {
	Vector2 center;
	float radius;
}Circle;

// キー入力結果を受け取る箱
Circle a, b;
Vector2 center = { 100,100 };
char keys[256] = { 0 };
char preKeys[256] = { 0 };
int color = RED;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	WSADATA wdData;
	static HANDLE hThread;
	static DWORD dwID;


	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, 1280, 720);

	hwMain = GetDesktopWindow();

	// 白い球
	a.center.x = 400;
	a.center.y = 400;
	a.radius = 100;

	// 赤い球
	b.center.x = 200;
	b.center.y = 200;
	b.radius = 50;

	// winsock初期化
	WSAStartup(MAKEWORD(2, 0), &wdData);

	// データを送受信処理をスレッド（WinMainの流れに関係なく動作する処理の流れ）として生成。
	// データ送受信をスレッドにしないと何かデータを受信するまでRECV関数で止まってしまう。
	hThread = (HANDLE)CreateThread(NULL, 0, &Threadfunc, (LPVOID)&a, 0, &dwID);

	// ウィンドウの×ボタンが押されるまでループ
	while (Novice::ProcessMessage() == 0) {
		// フレームの開始
		Novice::BeginFrame();

		// キー入力を受け取る
		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		if (keys[DIK_UP] != 0) {
			a.center.y -= 5;
		}
		if (keys[DIK_DOWN] != 0) {
			a.center.y += 5;
		}
		if (keys[DIK_RIGHT] != 0) {
			a.center.x += 5;
		}
		if (keys[DIK_LEFT] != 0) {
			a.center.x -= 5;
		}

		///
		/// ↓更新処理ここから
		///

		float distance =
			sqrtf((float)pow((double)a.center.x - (double)b.center.x, 2) +
				(float)pow((double)a.center.y - (double)b.center.y, 2));

		if (distance <= a.radius + b.radius) {
			color = BLUE;
		}
		else color = RED;
		///
		/// ↑更新処理ここまで
		///

		///
		/// ↓描画処理ここから
		///
		Novice::DrawEllipse((int)a.center.x, (int)a.center.y, (int)a.radius, (int)a.radius, 0.0f, WHITE, kFillModeSolid);
		Novice::DrawEllipse((int)b.center.x, (int)b.center.y, (int)b.radius, (int)b.radius, 0.0f, color, kFillModeSolid);
		///
		/// ↑描画処理ここまで
		///

		// フレームの終了
		Novice::EndFrame();

		// ESCキーが押されたらループを抜ける
		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
			break;
		}
	}

	// ライブラリの終了
	Novice::Finalize();

	// winsock終了
	WSACleanup();

	return 0;
}

/* 通信スレッド関数 */
DWORD WINAPI Threadfunc(void*) {

	SOCKET sConnect;
	SOCKADDR_IN saConnect;

	// 接続先サーバのIPアドレスとポート番号
	//char lpszAddr[] = "192.168.110.179";
	// アドレス直打ちをやめる
	// const char* addr = "192.168.***.***";
	char addr[20]; // IPアドレス用配列

	// ファイル読み込み (#include <fstream> が必要)
	std::ifstream ifs("ip.txt");
	ifs.getline(addr, sizeof(addr));

	WORD wPort = 8000;

	// ソケット作成
	sConnect = socket(PF_INET, SOCK_STREAM, 0);

	// コネクト用SOCKADDR_IN変数を初期化
	ZeroMemory(&saConnect, sizeof(SOCKADDR_IN));
	saConnect.sin_family = AF_INET;
	saConnect.sin_addr.s_addr = inet_addr(addr);
	saConnect.sin_port = htons(wPort);

	// サーバに接続要求
	if (connect(sConnect, (sockaddr*)(&saConnect), sizeof(saConnect)) == SOCKET_ERROR)
	{
		SetWindowText(hwMain, L"接続エラー");
		closesocket(sConnect);
		WSACleanup();

		return 1;
	}

	SetWindowText(hwMain, L"送信開始\r\n");


	while (1)
	{
		int     nRcv;

		// 送られてくる1Pの座標情報を退避
		//old_pos1P = pos1P;

		// 2Pの座標情報を送信
		send(sConnect, (const char*)&a, sizeof(Circle), 0);

		// サーバから1P座標情報を取得
		nRcv = recv(sConnect, (char*)&b, sizeof(Circle), 0);

		// 通信が途絶えたらループ終了
		if (nRcv == SOCKET_ERROR)
		{
			break;
		}

		//// 自分が動いてなくても、受信したサーバが操作するキャラの座標が更新されたら
		//// 更新領域を作ってInvalidateRect関数でWM_PAINTメッセージを発行して
		//// Window領域を再描画する
		//if (old_pos1P.x != pos1P.x || old_pos1P.y != pos1P.y)
		//{
		//	rect.left = old_pos1P.x - 10;
		//	rect.top = old_pos1P.y - 10;
		//	rect.right = old_pos1P.x + 42;
		//	rect.bottom = old_pos1P.y + 42;
		//	InvalidateRect(hwMain, &rect, TRUE);
		//}
	}

	closesocket(sConnect);

	return 0;
}
#if 0
// 通信スレッド関数
DWORD WINAPI threadfunc(void* px) {

	SOCKET sConnect;
	struct sockaddr_in saConnect, saLocal;
	DWORD dwAddr;
	// 送信バッファ
	Circle* lpBuf = new Circle;

	// サーバプログラムを実行するPCのIPアドレスを設定
//	const char* addr = "192.168.0.22";
	char addr[17];
	WORD wPort = 8000;

	ifstream ifs("ip.txt");
	ifs.getline(addr, 16);

	dwAddr = inet_addr(addr);

	sConnect = socket(PF_INET, SOCK_STREAM, 0);

	ZeroMemory(&saConnect, sizeof(sockaddr_in));
	ZeroMemory(&saLocal, sizeof(sockaddr_in));

	saLocal.sin_family = AF_INET;
	saLocal.sin_addr.s_addr = INADDR_ANY;
	saLocal.sin_port = 0;

	bind(sConnect, (LPSOCKADDR)&saLocal, sizeof(saLocal));

	saConnect.sin_family = AF_INET;
	saConnect.sin_addr.s_addr = dwAddr;
	saConnect.sin_port = htons(wPort);

	// サーバーに接続
	if (connect(sConnect, (sockaddr*)(&saConnect), sizeof(saConnect)) == SOCKET_ERROR)
	{
		//		SetWindowText(hwMain, "接続エラー");

		closesocket(sConnect);

		WSACleanup();

		return 1;
	}

	SetWindowText(hwMain, L"送信開始\r\n");

	// 通信スタート
	send(sConnect, (char*)&a, sizeof(Circle), 0);

	while (1)
	{
		int     nRcv;

		// 送られてくる1Pの座標情報を退避
//		recv_circle = pos1P;

		// サーバから1P座標情報を取得
		nRcv = recv(sConnect, (char*)&b, sizeof(Circle), 0);

		// 通信が途絶えたらループ終了
		if (nRcv == SOCKET_ERROR)
		{
			break;
		}

		// 2Pの座標情報を送信
		send(sConnect, (const char*)&a, sizeof(Circle), 0);
	}

	closesocket(sConnect);

	// 送信データバッファ解放
	delete lpBuf;

	return 0;
}
#endif