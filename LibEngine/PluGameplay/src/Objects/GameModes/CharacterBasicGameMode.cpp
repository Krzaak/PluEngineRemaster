//
// Created by Plutex on 7/20/26.
//

#include "PluEngine/Gameplay/Objects/GameModes/CharacterBasicGameMode.h"

#include "PluEngine/Gameplay/Objects/CharacterPuppet.h"

void Plu::CharacterBasicGameMode::OnSetupComponents()
{
    GameMode::OnSetupComponents();
    PuppetClass = CharacterPuppet::GetStaticClass();
}
