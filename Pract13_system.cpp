
#include <iostream>
#include <windows.h>
#include <vector>
#include <ctime>
#include <algorithm>
#include <random>

using namespace std;

struct Bayum {
    long long health = 9000000;
    int resist = 44;
    int damage = 73843;
    int specialDamage = 150000;
    int attackCooldown = 5;
    int specialCooldown = 10;
};

struct Player {
    long health = 500000;
    int damage = 12000;
    int specialDamage = 30000;
    long long OneShotDamage = 10000000000;
    int attackCooldown = 2;
    int specialCooldown = 5;
    int defense = 20;
    int dodgeChance = 15;
    char name[64];
};

HANDLE semaphore;
HANDLE specialEvent;

vector<long long> totalDamage;
Bayum boss;
vector<Player> players;
bool fightRunning = true;

int alivePlayers()
{
    int c = 0;
    for (size_t i = 0; i < players.size(); i++)
        if (players[i].health > 0)
            c++;
    return c;
}

DWORD WINAPI BossThread(LPVOID param)
{
    int timer = 0;

    while (fightRunning)
    {
        Sleep(boss.attackCooldown * 1000);


        if (!fightRunning)
        {
            break;
        }

        timer += boss.attackCooldown;

        if (timer >= boss.specialCooldown)
        {
            int count = alivePlayers();

            cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
            cout << "\nБосс бьёт специальной атакой\n";

            SetEvent(specialEvent);

            long dmg = boss.specialDamage * (1 - 0.05 * (count - 1));

            for (size_t i = 0; i < players.size(); i++)
            {
                if (players[i].health <= 0) continue;

                if (rand() % 100 < players[i].dodgeChance)
                {
                    cout << "\n" << players[i].name << " уклонился\n";
                    continue;
                }

                long real = dmg * (100 - players[i].defense) / 100;

                players[i].health -= real;

                cout << "\n" << players[i].name << " получил " << real << "\nHP игрока: " << players[i].health << endl;

                if (players[i].health <= 0)
                    cout << "\n" << players[i].name << " умер\n" << endl;;
            }

            timer = 0;
        }
        else
        {
            vector<int> alive;

            for (size_t i = 0; i < players.size(); i++)
                if (players[i].health > 0)
                    alive.push_back(i);

            if (alive.empty())
            {
                fightRunning = false;
                break;
            }

            int target = alive[rand() % alive.size()];

            if (rand() % 100 < players[target].dodgeChance)
            {
                cout << "Босс промахнулся " << players[target].name << endl;
            }
            else
            {
                long real = boss.damage * (100 - players[target].defense) / 100;

                players[target].health -= real;

                cout << "Босс ударил " << players[target].name << " на " << real << " урона" << "\nHP игрока: " << players[target].health << "\n" << endl;

                if (players[target].health <= 0)
                    cout << players[target].name << " умер\n";
            }

        }
        cout << "\n---------------------------------------------\n" << endl;
    }

    return 0;
}

DWORD WINAPI PlayerrThread(LPVOID param)
{
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(1, 100);

    int id = *(int*)param;
    int specialTimer = 0;

    while (fightRunning)
    {
        Sleep(players[id].attackCooldown * 1000);
        specialTimer += players[id].attackCooldown;


        WaitForSingleObject(semaphore, INFINITE);


        if (!fightRunning || boss.health <= 0 || players[id].health <= 0)
        {
            ReleaseSemaphore(semaphore, 1, NULL);
            break;
        }

        int vanshot = dist(gen);
        long long dmg;

        if (vanshot == 1)
        {
            dmg = players[id].OneShotDamage;
            cout << players[id].name << " УНИЧТОЖЕНИЕ\n";
        }
        else if (specialTimer >= players[id].specialCooldown)
        {
            dmg = players[id].specialDamage * (100 - boss.resist) / 100;
            cout << players[id].name << " спец атака " << dmg << endl;
            specialTimer = 0;
        }
        else
        {
            dmg = players[id].damage * (100 - boss.resist) / 100;
            cout << players[id].name << " нанес " << dmg << endl;
        }

        boss.health -= dmg;
        totalDamage[id] += dmg;

        cout << "HP босса: " << boss.health << "\n" << endl;

        if (boss.health <= 0)
            fightRunning = false;

        ReleaseSemaphore(semaphore, 1, NULL);
    }

    return 0;
}

void Loaded()
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    system("cls");
    SetConsoleTextAttribute(hConsole, 14);
    cout << "Игра загружается...\n";
    Sleep(2000);

    system("cls");
    SetConsoleTextAttribute(hConsole, 10);
    cout << "Игра загрузилась\n";
    Sleep(1500);

    system("cls");
    SetConsoleTextAttribute(hConsole, 7);
}

int main()
{
    setlocale(0, "ru");
    srand((unsigned)time(NULL));


    specialEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    int n;
    semaphore = CreateSemaphore(NULL, 1, 1, NULL);
    cout << "Сколько игроков (1-10): ";
    cin >> n;
    if (n < 1) {
        n = 1;
    }
    if (n > 10) {
        n = 10;
    }
    players.resize(n);
    totalDamage.resize(n);

    for (int i = 0; i < n; i++)
    {
        cout << "Введите имя игрока " << i + 1 << ": ";
        cin >> players[i].name;
    }

    Loaded();
    cout << "----------- Boss Fight Simulation -----------\n" << endl;


    vector<HANDLE> threads;

    HANDLE bossThread = CreateThread(NULL, 0, BossThread, NULL, 0, NULL);
    threads.push_back(bossThread);

    vector<int> ids(n);

    for (int i = 0; i < n; i++)
    {
        ids[i] = i;
        HANDLE t = CreateThread(NULL, 0, PlayerrThread, &ids[i], 0, NULL);
        threads.push_back(t);
    }

    WaitForMultipleObjects((DWORD)threads.size(), threads.data(), TRUE, INFINITE);

    cout << "---------------- Результат ------------------" << endl;

    if (boss.health <= 0)
        cout << "Игроки победили\n";
    else
        cout << "Босс победил\n";

    vector<int> order(players.size());

    for (int i = 0; i < players.size(); i++)
        order[i] = i;

    for (int i = 0; i < players.size() - 1; i++)
    {
        for (int j = 0; j < players.size() - i - 1; j++)
        {
            if (totalDamage[order[j]] < totalDamage[order[j + 1]])
            {
                int temp = order[j];
                order[j] = order[j + 1];
                order[j + 1] = temp;
            }
        }
    }
    Sleep(2000);
    cout << "\nТоп-3 игрока по урону:\n";

    for (int i = 0; i < 3 && i < players.size(); i++)
    {
        int id = order[i];
        cout << i + 1 << " место " << players[id].name << " : " << totalDamage[id] << endl;
    }

    CloseHandle(semaphore);
    CloseHandle(specialEvent);

    return 0;
}
