//
// Created by Plutex on 7/20/26.
//

#include "PluEngine/BasicEngineClasses/GameObjects/GameModes/CharacterBasicGameMode.h"

#include "PluEngine/BasicEngineClasses/GameObjects/CharacterPuppet.h"

void Plu::CharacterBasicGameMode::OnSetupComponents()
{
    GameMode::OnSetupComponents();
    PuppetClass = CharacterPuppet::GetStaticClass();
}
