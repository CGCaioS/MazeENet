#include "GameplayState.h"

#include <iostream>
#include <conio.h>
#include <windows.h>
#include <assert.h>
#include <sstream>

#include "Enemy.h"
#include "Key.h"
#include "Door.h"
#include "Money.h"
#include "Goal.h"
#include "AudioManager.h"
#include "Utility.h"
#include "StateMachineExampleGame.h"

using namespace std;

constexpr int kArrowInput = 224;
constexpr int kLeftArrow = 75;
constexpr int kRightArrow = 77;
constexpr int kUpArrow = 72;
constexpr int kDownArrow = 80;
constexpr int kEscapeKey = 27;

GameplayState::GameplayState(StateMachineExampleGame* pOwner)
	: m_pOwner(pOwner)
	, m_beatLevel(false)
	, m_skipFrameCount(0)
	, m_currentLevel(0)
	, m_pLevel(nullptr)
	, m_player(true)
{
	m_LevelNames.push_back("Level1.txt");
	m_LevelNames.push_back("Level2.txt");
	m_LevelNames.push_back("Level3.txt");

	auto getInput = []()->int {
		return _getch();
	};

	m_inputFuture = std::async(std::launch::async, getInput);
}

GameplayState::~GameplayState()
{
	delete m_pLevel;
	m_pLevel = nullptr;

	for (auto otherPlayerPair : m_otherPlayers)
	{
		if (otherPlayerPair.second != nullptr)
		{
			Player* otherPlayer = otherPlayerPair.second;
			delete otherPlayer;
			otherPlayer = nullptr;
		}
	}

	m_otherPlayers.clear();
}

bool GameplayState::Load()
{
	if (m_pLevel)
	{
		delete m_pLevel;
		m_pLevel = nullptr;
	}

	m_pLevel = new Level();
	
	return m_pLevel->Load(m_LevelNames.at(m_currentLevel), m_player.GetXPositionPointer(), m_player.GetYPositionPointer());

}

void GameplayState::Enter()
{
	Load();
}

bool GameplayState::Update(bool processInput)
{
	
	if (processInput && !m_beatLevel)
	{
		ProcessENetMessages();
		

		// sends _getch to a different thread and only process input when a key is pressed
		if (m_inputFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			auto input = m_inputFuture.get();

			auto getInput = []()->int {
				return _getch();
			};

			m_inputFuture = std::async(std::launch::async, getInput);					

			int arrowInput = 0;
			int newPlayerX = m_player.GetXPosition();
			int newPlayerY = m_player.GetYPosition();

			// One of the arrow keys were pressed
			
			if ((input == kArrowInput && arrowInput == kLeftArrow) ||
				(char)input == 'A' || (char)input == 'a')
			{
				newPlayerX--;
			}
			else if ((input == kArrowInput && arrowInput == kRightArrow) ||
				(char)input == 'D' || (char)input == 'd')
			{
				newPlayerX++;
			}
			else if ((input == kArrowInput && arrowInput == kUpArrow) ||
				(char)input == 'W' || (char)input == 'w')
			{
				newPlayerY--;
			}
			else if ((input == kArrowInput && arrowInput == kDownArrow) ||
				(char)input == 'S' || (char)input == 's')
			{
				newPlayerY++;
			}
			else if (input == kEscapeKey)
			{
				m_pOwner->LoadScene(StateMachineExampleGame::SceneName::MainMenu);
			}
			else if ((char)input == 'Z' || (char)input == 'z')
			{
				m_player.DropKey();
			}

			// If position never changed
			if (newPlayerX == m_player.GetXPosition() && newPlayerY == m_player.GetYPosition())
			{
				//return false;
			}
			else
			{
				HandleCollision(newPlayerX, newPlayerY);
			}

			// Boradcasts current position to other players
			std::stringstream stringStream;

			stringStream << ENetClient::GetInstance().GetPeerID() << "-" << m_player.GetXPosition() << "," << m_player.GetYPosition();

			ENetClient::GetInstance().Send(DeliveryType::RELIABLE, stringStream.str());
		}
		
		
	}
	if (m_beatLevel)
	{
		++m_skipFrameCount;
		if (m_skipFrameCount > kFramesToSkip)
		{
			m_beatLevel = false;
			m_skipFrameCount = 0;
			++m_currentLevel;
			if (m_currentLevel == m_LevelNames.size())
			{
				Utility::WriteHighScore(m_player.GetMoney());

				AudioManager::GetInstance()->PlayWinSound();
				
				m_pOwner->LoadScene(StateMachineExampleGame::SceneName::Win);
			}
			else
			{
				// On to the next level
				Load();
			}

		}
	}

	return false;
}

void GameplayState::HandleCollision(int newPlayerX, int newPlayerY)
{
	PlacableActor* collidedActor = m_pLevel->UpdateActors(newPlayerX, newPlayerY);
	if (collidedActor != nullptr && collidedActor->IsActive())
	{
		switch (collidedActor->GetType())
		{
		case ActorType::Enemy:
		{
			Enemy* collidedEnemy = dynamic_cast<Enemy*>(collidedActor);
			assert(collidedEnemy);
			AudioManager::GetInstance()->PlayLoseLivesSound();
			collidedEnemy->Remove();
			m_player.SetPosition(newPlayerX, newPlayerY);

			m_player.DecrementLives();
			if (m_player.GetLives() < 0)
			{
				//TODO: Go to game over screen
				AudioManager::GetInstance()->PlayLoseSound();
				m_pOwner->LoadScene(StateMachineExampleGame::SceneName::Lose);
			}
			break;
		}
		case ActorType::Money:
		{
			Money* collidedMoney = dynamic_cast<Money*>(collidedActor);
			assert(collidedMoney);
			AudioManager::GetInstance()->PlayMoneySound();
			collidedMoney->Remove();
			m_player.AddMoney(collidedMoney->GetWorth());
			m_player.SetPosition(newPlayerX, newPlayerY);
			break;
		}
		case ActorType::Key:
		{
			Key* collidedKey = dynamic_cast<Key*>(collidedActor);
			assert(collidedKey);
			if (!m_player.HasKey())
			{
				m_player.PickupKey(collidedKey);
				collidedKey->Remove();
				m_player.SetPosition(newPlayerX, newPlayerY);
				AudioManager::GetInstance()->PlayKeyPickupSound();
			}
			break;
		}
		case ActorType::Door:
		{
			Door* collidedDoor = dynamic_cast<Door*>(collidedActor);
			assert(collidedDoor);
			if (!collidedDoor->IsOpen())
			{
				if (m_player.HasKey(collidedDoor->GetColor()))
				{
					collidedDoor->Open();
					collidedDoor->Remove();
					m_player.UseKey();
					m_player.SetPosition(newPlayerX, newPlayerY);
					AudioManager::GetInstance()->PlayDoorOpenSound();
				}
				else
				{
					AudioManager::GetInstance()->PlayDoorClosedSound();
				}
			}
			else
			{
				m_player.SetPosition(newPlayerX, newPlayerY);
			}
			break;
		}
		case ActorType::Goal:
		{
			Goal* collidedGoal = dynamic_cast<Goal*>(collidedActor);
			assert(collidedGoal);
			collidedGoal->Remove();
			m_player.SetPosition(newPlayerX, newPlayerY);
			m_beatLevel = true;
			break;
		}
		default:
			break;
		}
	}
	else if (m_pLevel->IsSpace(newPlayerX, newPlayerY)) // no collision
	{
		m_player.SetPosition(newPlayerX, newPlayerY);
	}
	else if (m_pLevel->IsWall(newPlayerX, newPlayerY))
	{
		// wall collision, do nothing
	}
}

void GameplayState::Draw()
{
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	system("cls");

	m_pLevel->Draw();

	// Set cursor position for player 
	COORD actorCursorPosition;
	actorCursorPosition.X = m_player.GetXPosition();
	actorCursorPosition.Y = m_player.GetYPosition();
	SetConsoleCursorPosition(console, actorCursorPosition);
	m_player.Draw();

	for (const auto& otherPlayerPair : m_otherPlayers)
	{
		Player* otherPlayer = otherPlayerPair.second;

		if (otherPlayer != nullptr)
		{
			COORD cursorPosition;
			cursorPosition.X = otherPlayer->GetXPosition();
			cursorPosition.Y = otherPlayer->GetYPosition();
			SetConsoleCursorPosition(console, cursorPosition);
			otherPlayer->Draw();
		}
		
	}

	// Set the cursor to the end of the level
	COORD currentCursorPosition;
	currentCursorPosition.X = 0;
	currentCursorPosition.Y = m_pLevel->GetHeight();
	SetConsoleCursorPosition(console, currentCursorPosition);

	DrawHUD(console);
}

void GameplayState::DrawHUD(const HANDLE& console)
{
	cout << endl;

	// Top Border
	for (int i = 0; i < m_pLevel->GetWidth(); ++i)
	{
		cout << Level::WAL;
	}
	cout << endl;

	// Left Side border
	cout << Level::WAL;

	cout << " wasd-move " << Level::WAL << " z-drop key " << Level::WAL;

	cout << " $:" << m_player.GetMoney() << " " << Level::WAL;
	cout << " lives:" << m_player.GetLives() << " " << Level::WAL;
	cout << " key:";
	if (m_player.HasKey())
	{
		m_player.GetKey()->Draw();
	}
	else
	{
		cout << " ";
	}

	// RightSide border
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(console, &csbi);

	COORD pos;
	pos.X = m_pLevel->GetWidth() - 1;
	pos.Y = csbi.dwCursorPosition.Y;
	SetConsoleCursorPosition(console, pos);

	cout << Level::WAL;
	cout << endl;

	// Bottom Border
	for (int i = 0; i < m_pLevel->GetWidth(); ++i)
	{
		cout << Level::WAL;
	}
	cout << endl;
}

void GameplayState::ProcessENetMessages()
{
	// poll for messages
	const auto& messages = ENetClient::GetInstance().Poll();

	// process messages
	for (const auto& msg : messages)
	{
		int id = msg.GetPeerID();

		switch (msg.GetType())
		{
			case Message::Type::DISCONNECT:

				// TODO: handle the disconnect
				break;

			case Message::Type::DATA:

				std::string dataStr = msg.GetData();

				//split string
				const auto dataPrefixPos = dataStr.find("-");

				std::string peerIDStr = dataStr.substr(0, dataPrefixPos);
				int peerID = atoi(peerIDStr.c_str());

				if (peerID != ENetClient::GetInstance().GetPeerID())
				{
					if (m_otherPlayers.count(id) == 0)
					{
						Player* otherPlayer = new Player(false);
						m_otherPlayers[id] = otherPlayer;
					}

					Player* otherPlayer = m_otherPlayers.at(id);

					std::string positionStr = dataStr.substr(dataPrefixPos + 1, dataStr.size() - peerIDStr.size() - 1);

					const auto findPos = positionStr.find(",");

					std::string strX = positionStr.substr(0, findPos);
					std::string strY = positionStr.substr(findPos + 1, positionStr.size() - strX.size() - 1);

					otherPlayer->SetPosition(atoi(strX.c_str()), atoi(strY.c_str()));
				}
							

			break;
		}
	}
}
