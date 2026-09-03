#ifndef PLUSTL_FWD_H
#define PLUSTL_FWD_H

#include "Pointers/ControlBlock.h"
#include "Pointers/Casts.h"

//Pointers
#include "Pointers/TOwningPointer.h"
#include "Pointers/TUsePointer.h"

//Hashers
#include "Hashers/Default.h"
#include "Hashers/String.h"

//Random
#include "Random/Random.h"

//Arrays
#include "Array/Array.h"

//Queues
#include "Queue/Queue.h"

//Strings
#include "String/String.h"
#include "Path/Path.h"

//HashMaps
#include "HashMap/HashMap.h"
#include "HashMap/HashMapV2.h"
//HashSet
#include "HashSet/HashSet.h"

// Concurrent/ is deliberately NOT included here. This header is the precompiled header
// for both Engine (LibEngine/CMakeLists.txt) and PluEditor (Editor/CMakeLists.txt), so
// anything added here lands in every translation unit in the project — and the concurrent
// containers pull in <atomic>, <mutex>, <shared_mutex> and <thread>. Only a handful of
// files need them, and those include the specific header they use:
//   #include "Concurrent/ConcurrentHashMap.h"   (also: ...HashSet / ...Queue / ...Array /
//                                                ...String / LockPrimitives.h, or
//                                                Concurrent/Concurrent.h for all of them)
// Please do not "fix" this by adding them above.
//
// (ConcurrentHashMap.h does reach every engine TU anyway, because Profiler.h holds one and
// Timer.h — itself a PCH — includes Profiler.h. That is one header by an explicit decision;
// it is not a reason to make the other four unconditional, and PluSTL consumers outside the
// engine, e.g. Tests/, pay for none of them.)

#endif
