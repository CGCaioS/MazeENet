#pragma once
#include "GameState.h"
#include "Player.h"
#include "Level.h"

#include "ENetClient.h"

#include <windows.h>
#include <vector>
#include <string>

#include <map>

#include <future>

class StateMachineExampleGame;

class GameplayState : public GameState
{
	StateMachineExampleGame* m_pOwner;
	
	Player m_player;
	Level* m_pLevel;

	bool m_beatLevel;
	int m_skipFrameCount;
	static constexpr int kFramesToSkip = 2;

	int m_currentLevel;

	std::vector<std::string> m_LevelNames;

public:
	GameplayState(StateMachineExampleGame* pOwner);
	~GameplayState();

	virtual void Enter() override;
	virtual bool Update(bool processInput = true) override;
	virtual void Draw() override;

private:
	void HandleCollision(int newPlayerX, int newPlayerY);
	bool Load();
	void DrawHUD(const HANDLE& console);

	void ProcessENetMessages();

	std::future<int> m_inputFuture;

	std::map<int, Player*> m_otherPlayers;
};
