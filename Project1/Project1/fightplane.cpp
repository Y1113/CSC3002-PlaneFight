#include <iostream>
#include <graphics.h>
#include <vector>
#include <conio.h>
#include <tchar.h>
#include <string>
#include <memory>
#include <random>
using namespace std;
// 定义飞机的宽高，游戏屏幕的宽高
constexpr auto Plane_width = 75;
constexpr auto Plane_height = 75;
constexpr auto swidth = 600;
constexpr auto sheight = 900;
// 飞机生命值
constexpr unsigned int SHP = 5;
//受伤后的无敌时间，毫秒
constexpr auto hurttime = 1500;

bool PointInRect(int x, int y, const RECT& r) {
	return r.left <= x && x <= r.right && r.top <= y && y <= r.bottom;
}


bool ChceckCollision(const RECT& r1, const RECT& r2) {
	return !(r1.right < r2.left ||
		r1.left > r2.right ||
		r1.bottom < r2.top ||
		r1.top > r2.bottom);
}


void DrawTextCentered(const TCHAR* text, int y, int fontSize, COLORREF color) {
	settextstyle(fontSize, 0, _T("黑体"));
	settextcolor(color);
	outtextxy(swidth / 2 - textwidth(text) / 2, y, text);
}

void Start() {
	const TCHAR* title = _T("Fight Plane");
	const TCHAR* tstart = _T("Start Game");
	const TCHAR* texit = _T("Quit Game");
	const TCHAR* names[] = { _T("杨卓灵 122040092"), _T("王  晟 120090813"), _T("钟佩容 122090809") };

	RECT tstartr, texitr;
	BeginBatchDraw();
	setbkcolor(WHITE);
	cleardevice();

	DrawTextCentered(title, sheight / 5, 60, BLACK);

	tstartr = {
		static_cast<LONG>(swidth / 2 - textwidth(tstart) / 2),
		static_cast<LONG>(sheight / 5 * 2.5),
		static_cast<LONG>(swidth / 2 + textwidth(tstart) / 2),
		static_cast<LONG>(sheight / 5 * 2.5 + textheight(tstart))
	};

	DrawTextCentered(tstart, sheight / 5.0 * 2.5, 40, BLACK);

	texitr = { swidth / 2 - textwidth(texit) / 2, sheight / 5 * 3, swidth / 2 + textwidth(texit) / 2, sheight / 5 * 3 + textheight(texit) };
	DrawTextCentered(texit, sheight / 5.0 * 3, 40, BLACK);

	int x = swidth - 10;
	int y = sheight - textheight(names[0]) * 3 - 10;
	for (const TCHAR* name : names) {
		outtextxy(x - textwidth(name), y, name);
		y += textheight(name);
	}

	EndBatchDraw();

	while (true) {
		ExMessage mess;
		getmessage(&mess, EM_MOUSE);
		if (mess.lbutton) {
			if (PointInRect(mess.x, mess.y, tstartr)) {
				return;
			}
			else if (PointInRect(mess.x, mess.y, texitr)) {
				exit(0);
			}
		}
	}
}

void Over(unsigned long long& kill) {
	TCHAR str[128];
	_stprintf_s(str, _T("Kills: %llu"), kill);
	DrawTextCentered(str, sheight / 5, 60, RED);
	//鼠标点击任何位置返回菜单
	const TCHAR* info = _T("Click anywhere to return to the menu");
	DrawTextCentered(info, sheight - textheight(info), 20, BLACK);

	while (true) {
		ExMessage msg;
		if (peekmessage(&msg, EM_MOUSE)) {  
			if (msg.message == WM_LBUTTONDOWN) {  
				break;  
			}
		}
		Sleep(10);  
	}
}

// 背景、敌机、英雄、子弹

class BK {
public:
	BK(IMAGE& img, int scrollSpeed = 3)
		: img(img), y(-sheight), scrollSpeed(scrollSpeed) {
	}

	void Show() {
		y += scrollSpeed;  // 调整背景滚动速度
		if (y >= 0) {
			y -= sheight;
		}
		putimage(0, y, &img);  // 显示背景
		if (y > -sheight) {
			putimage(0, y - sheight, &img);  // 绘制顶部接续的背景，确保无缝滚动
		}
	}

private:
	IMAGE& img;
	int y;
	int scrollSpeed;
};

enum class Direction {
	Up,
	Down,
	Left,
	Right
};

class Plane
{
public:
	Plane(IMAGE& img)
		: img(img), HP(SHP), speed(30), // speed 可以根据实际需要调整
		rect{ swidth / 2 - img.getwidth() / 2, sheight - img.getheight(),
			 swidth / 2 + img.getwidth() / 2, sheight } {}

	void Show() {
		setlinecolor(RED);
		setlinestyle(PS_SOLID, 4);
		putimage(rect.left, rect.top, &img);
		line(rect.left, rect.top - 5, rect.left + (img.getwidth() / SHP * HP), rect.top - 5);
	}

	void Control() {
		if (_kbhit()) {
			int ch = _getch();
			Direction direction = GetDirection(ch);
			Move(direction);
		}
	}

	void Move(Direction direction) {
		switch (direction) {
		case Direction::Up:
			rect.top = max(0, rect.top - speed);
			rect.bottom = rect.top + img.getheight();
			break;
		case Direction::Down:
			rect.top = min(sheight - img.getheight(), rect.top + speed);
			rect.bottom = rect.top + img.getheight();
			break;
		case Direction::Left:
			rect.left = max(0, rect.left - speed);
			rect.right = rect.left + img.getwidth();
			break;
		case Direction::Right:
			rect.left = min(swidth - img.getwidth(), rect.left + speed);
			rect.right = rect.left + img.getwidth();
			break;
		}
	}

	Direction GetDirection(int keycode) {
		switch (keycode) {
		case 'w': case 'W': return Direction::Up;
		case 's': case 'S': return Direction::Down;
		case 'a': case 'A': return Direction::Left;
		case 'd': case 'D': return Direction::Right;
		default: return Direction::Down; 
		}
	}


	bool hurt()
	{
		HP--;
		return (HP == 0) ? false : true;
	}

	RECT& GetRect() { return rect; }

private:
	IMAGE& img;
	RECT rect;

	unsigned int HP;
	int speed;
};


class Enemy {
public:
	// 构造函数，接受爆炸帧的图像向量。
	Enemy(IMAGE& img, int x, const std::vector<IMAGE>& boomFrames)
		: img(img), boomFrames(boomFrames), isDead(false), boomIndex(0),
		rect{ x, -img.getheight(), x + img.getwidth(), 0 } {
	}

	// 显示敌人或爆炸序列
	bool Show() {
		if (isDead) {
			if (boomIndex < boomFrames.size()) {
				putimage(rect.left, rect.top, &boomFrames[boomIndex++]);
				return true; // 继续显示爆炸帧
			}
			return false; // 所有爆炸帧显示完毕
		}

		// 每帧向下移动敌人直到屏幕外
		rect.top += 4;
		rect.bottom += 4;
		if (rect.top >= sheight) {
			return false; // 敌人移出屏幕
		}

		putimage(rect.left, rect.top, &img);
		return true;
	}

	// 触发死亡序列
	void die() {
		isDead = true;
		boomIndex = 0; // 重置索引开始爆炸序列
	}

	// 获取敌人的矩形用于碰撞检测
	RECT& GetRect() {
		return rect;
	}

private:
	IMAGE& img; // 敌人图像的引用
	std::vector<IMAGE> boomFrames; // 爆炸序列帧
	bool isDead; // 标志敌人是否死亡
	int boomIndex; // 爆炸序列的当前帧
	RECT rect; // 定义敌人位置和大小的矩形
};

class Bullet {
public:
	static const int Speed = 3;  // 定义子弹的速度，每次移动的像素数

	Bullet(IMAGE& img, const RECT& planeRect) : img(img) {
		// 计算子弹的起始位置，使其在飞机中央底部发射
		int bulletWidth = img.getwidth();
		int bulletHeight = img.getheight();
		int centerX = (planeRect.left + planeRect.right) / 2;
		rect.left = centerX - bulletWidth / 2;
		rect.right = rect.left + bulletWidth;
		rect.top = planeRect.top - bulletHeight;
		rect.bottom = rect.top + bulletHeight;
	}

	// 更新子弹的位置并显示，返回是否还应该保留子弹
	bool Update() {
		// 更新子弹位置
		rect.top -= Speed;
		rect.bottom -= Speed;

		// 如果子弹已经完全移出屏幕，则返回false
		if (rect.bottom <= 0) {
			return false;
		}

		// 显示子弹
		putimage(rect.left, rect.top, &img);
		return true;
	}

	// 获取子弹的位置
	const RECT& GetRect() const { return rect; }

public:
	IMAGE& img; // 子弹图像
	RECT rect;  // 子弹当前位置
};


class EBullet : public Bullet {
public:
	static const int Speed = 5;  // 敌方子弹的移动速度，每次移动的像素数

	// 构造函数，初始化敌方子弹的位置和图像
	EBullet(IMAGE& img, const RECT& enemyRect) : Bullet(img, enemyRect) {
		// 设置子弹发射点在敌机中心底部
		int centerX = (enemyRect.left + enemyRect.right) / 2;
		rect.left = centerX - img.getwidth() / 2;
		rect.right = rect.left + img.getwidth();
		rect.top = enemyRect.bottom;  // 敌机底部发射
		rect.bottom = rect.top + img.getheight();
	}

	// 更新子弹的位置并显示，返回是否还应该保留子弹
	bool Update() {
		// 更新子弹位置，向下移动
		rect.top += Speed;
		rect.bottom += Speed;

		// 如果子弹已经完全移出屏幕，则返回false
		if (rect.top >= sheight) {
			return false;
		}

		// 显示子弹
		putimage(rect.left, rect.top, &img);
		return true;
	}
};



bool AddEnemy(vector<std::unique_ptr<Enemy>>& es, IMAGE& enemyimg, std::vector<IMAGE>& boomFrames) {
	std::random_device rd; // 非确定性随机数生成器
	std::mt19937 gen(rd()); // 以 rd() 为种子的 Mersenne Twister 生成器
	std::uniform_int_distribution<> dist(0, swidth - enemyimg.getwidth()); // 敌人位置的分布

	int x = dist(gen); // 获取一个随机的 x 位置

	// 确保使用与构造函数匹配的参数类型
	es.emplace_back(std::make_unique<Enemy>(enemyimg, x, boomFrames));
	return true;
}



bool Play()
{
	setbkcolor(WHITE);
	cleardevice();
	bool is_play = true;
	std::vector<IMAGE> boomFrames;
	IMAGE Planeimg, enemyimg, bkimg, bulletimg, boom1, boom2, boom3;
	loadimage(&Planeimg, _T("C:/Users/Administrator/Desktop/PlaneFight/images/plane.png"), Plane_width, Plane_height, true);
	loadimage(&enemyimg, _T("C:/Users/Administrator/Desktop/PlaneFight/images/enemy.png"));
	loadimage(&bkimg, _T("C:/Users/Administrator/Desktop/PlaneFight/images/bg.png"), swidth, sheight * 2);
	loadimage(&bulletimg, _T("C:/Users/Administrator/Desktop/PlaneFight/images/bullet.png"));

	loadimage(&boom1, _T("C:/Users/Administrator/Desktop/PlaneFight/images/e_down1.png"));
	loadimage(&boom2, _T("C:/Users/Administrator/Desktop/PlaneFight/images/e_down2.png"));
	loadimage(&boom3, _T("C:/Users/Administrator/Desktop/PlaneFight/images/e_down3.png"));
	boomFrames.push_back(boom1);
	boomFrames.push_back(boom2);
	boomFrames.push_back(boom3);
	BK bk = BK(bkimg);
	Plane hp = Plane(Planeimg);

	vector<std::unique_ptr<Enemy>> es;
	vector<Bullet*> bs;
	vector<EBullet*> ebs;
	int bsing = 0;

	clock_t hurtlast = clock();

	unsigned long long kill = 0;

	for (int i = 0; i < 5; i++)
	{
		AddEnemy(es, enemyimg, boomFrames);
	}

	while (is_play)
	{
		bsing++;
		if (bsing % 10 == 0)
		{
			bs.push_back(new Bullet(bulletimg, hp.GetRect()));
		}
		if (bsing == 60)
		{
			bsing = 0;
			for (auto& i : es)
			{
				ebs.push_back(new EBullet(bulletimg, i->GetRect()));
			}
		}

		BeginBatchDraw();

		bk.Show();
		Sleep(2);
		flushmessage();
		Sleep(2);
		hp.Control();

		if (_kbhit())
		{
			char v = _getch();
			if (v == 0x20)
			{
				Sleep(500);
				while (true)
				{
					if (_kbhit())
					{
						v = _getch();
						if (v == 0x20)
						{
							break;
						}
					}
					Sleep(16);
				}
			}
		}
		hp.Show();

		auto bsit = bs.begin();
		while (bsit != bs.end())
		{
			if (!(*bsit)->Update())
			{
				bsit = bs.erase(bsit);
			}
			else
			{
				bsit++;
			}
		}

		auto ebsit = ebs.begin();
		while (ebsit != ebs.end())
		{
			if (!(*ebsit)->Update())
			{
				ebsit = ebs.erase(ebsit);
			}
			else
			{
				if (ChceckCollision((*ebsit)->GetRect(), hp.GetRect()))
				{
					if (clock() - hurtlast >= hurttime)
					{
						is_play = hp.hurt();
						hurtlast = clock();
					}
				}
				ebsit++;
			}

		}

		auto it = es.begin();
		while (it != es.end())
		{
			if (ChceckCollision((*it)->GetRect(), hp.GetRect()))
			{
				if (clock() - hurtlast >= hurttime)
				{
					is_play = hp.hurt();
					hurtlast = clock();
				}
			}
			auto bit = bs.begin();
			while (bit != bs.end())
			{
				if (ChceckCollision((*bit)->GetRect(), (*it)->GetRect()))
				{
					(*it)->die();
					bit = bs.erase(bit);  //自动删除子弹

					kill++;
					break;
				}
				else {
					++bit;
				}
			}
			if (!(*it)->Show()) {
				it = es.erase(it);  // 自动删除敌人
			}
			else {
				++it;
			}
		}
		for (int i = 0; i < 5 - es.size(); i++)
		{
			AddEnemy(es, enemyimg, boomFrames);
		}

		EndBatchDraw();
	}
	printf_s("e");
	Over(kill);

	return true;
}

int main()
{
	initgraph(swidth, sheight, EW_NOMINIMIZE | EW_SHOWCONSOLE);
	bool is_live = true;
	while (is_live)
	{
		Start();
		is_live = Play();
	}

	return 0;
}