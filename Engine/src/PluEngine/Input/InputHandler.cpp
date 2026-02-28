//
// Created by Plutex on 2026-02-27.
//

#include "PluEngine/Input/InputHandler.h"

Plu::InputHandler::~InputHandler()
{
	mHoldActions.Clear();
	mPressedActions.Clear();
	mReleasedActions.Clear();
}

void Plu::InputHandler::TickHandler()
{
	for (auto key : mKeyboard) {
		if (key.second != ButtonState::Held) continue;
		if (mHoldActions.Contains(key.first)) {
			mHoldActions[key.first]();
		}
	}
}

void Plu::InputHandler::UpdateKeyState(Key key, ButtonState state)
{
	mKeyboard[key] = state;
	if (state == ButtonState::Pressed && mPressedActions.Contains(key)) {
		mPressedActions[key]();
	}
	if (state == ButtonState::JustReleased && mReleasedActions.Contains(key)) {
		mReleasedActions[key]();
	}
}

void Plu::InputHandler::AddActionOnPress(Key key, std::function<void()> callback)
{
	mPressedActions[key] = callback;
}

void Plu::InputHandler::AddActionOnRelease(Key key, std::function<void()> callback)
{
	mReleasedActions[key] = callback;
}

void Plu::InputHandler::AddActionOnHold(Key key, std::function<void()> callback)
{
	mHoldActions[key] = callback;
}
