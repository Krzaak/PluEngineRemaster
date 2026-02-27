//
// Created by Plutex on 2026-02-12.
//

#ifndef PLUENGINE_GAMECLIENT_H
#define PLUENGINE_GAMECLIENT_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "GameClient.generated.h"

namespace Plu
{
	class InputManager;
	PLU_CLASS(Abstract)
	class PLU_API GameClient : public EngineObject
	{
		REFLECTION_BODY_GAMECLIENT()
	private:
		DynamicArray<TOwningPointer<class GameLocalPlayer>> mLocalPlayers;
		TUsePointer<EngineObjectManager> mObjectManager;
		TUsePointer<IScenesManager> mScenesManager;
		TUsePointer<InputManager> mInputManager;

		friend class SceneWorld;
	public:
		GameClient(const TUsePointer<EngineObjectManager> &objectManager, const TUsePointer<IScenesManager> &scenesManager, const TUsePointer<InputManager> &inputManager);
		~GameClient() override;

		void ExitGame();

		//Returns UInt16 as player ID, by this id you can get player controller, puppet and every object associated with that player
		//For now we only support one player
		TUsePointer<GameLocalPlayer> GetLocalPlayerByID(UInt16 id);
		UInt16 JoinGameLocally();
	};
}

#endif //PLUENGINE_GAMECLIENT_H
