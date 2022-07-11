#include "Game.h"
#include <thread>
#include <chrono>

#include "ENetClient.h"

const std::string HOST = "localhost";
const uint32_t PORT = 7000;

Game::Game()
	: m_pStateMachine(nullptr)
{

}

void Game::Initialize(GameStateMachine* pStateMachine)
{
	if (pStateMachine != nullptr)
	{
		pStateMachine->Init();
		m_pStateMachine = pStateMachine;
	}
}

void Game::RunGameLoop()
{
	bool isGameOver = false;

	ENetClient::GetInstance().Connect(HOST, PORT);

	while (!isGameOver)
	{
		// update with no input
		Update(false);
		// Draw
		Draw();
		// Update with input
		isGameOver = Update();

		std::this_thread::sleep_for(std::chrono::milliseconds(33)); // 30 fps
	}

	Draw();
}

void Game::Deinitialize()
{
	if (m_pStateMachine != nullptr)
		m_pStateMachine->Cleanup();
}

bool Game::Update(bool processInput)
{
	return m_pStateMachine->UpdateCurrentState(processInput);
}

void Game::Draw()
{
	m_pStateMachine->DrawCurrentState();
}
