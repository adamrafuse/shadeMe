#include "ShadowFacts.h"
#include "ShadowFigures.h"
#include "ShadowSundries.h"

#pragma warning(disable: 4005 4748)

namespace ShadowFacts
{
	UInt32* ShadowCasterCountTable::GetCurrentCount( UInt8 Type )
	{
		switch (Type)
		{
		case kFormType_NPC:
		case kFormType_Creature:
			return &Current[kMaxShadows_Actor];
		case kFormType_Book:
			return &Current[kMaxShadows_Book];
		case kFormType_Flora:
			return &Current[kMaxShadows_Flora];
		case kFormType_Ingredient:
		case kFormType_SigilStone:
		case kFormType_SoulGem:
			return &Current[kMaxShadows_Ingredient];
		case kFormType_Misc:
		case kFormType_Key:
			return &Current[kMaxShadows_MiscItem];
		case kFormType_AlchemyItem:
			return &Current[kMaxShadows_AlchemyItem];
		case kFormType_Ammo:
		case kFormType_Armor:
		case kFormType_Clothing:
		case kFormType_Weapon:
			return &Current[kMaxShadows_Equipment];
		default:
			return NULL;
		}
	}

	SInt32 ShadowCasterCountTable::GetMaxCount( UInt8 Type ) const
	{
		switch (Type)
		{
		case kFormType_NPC:
		case kFormType_Creature:
			return Settings::kMaxCountActor().i;
		case kFormType_Book:
			return Settings::kMaxCountBook().i;
		case kFormType_Flora:
			return Settings::kMaxCountFlora().i;
		case kFormType_Ingredient:
		case kFormType_SigilStone:
		case kFormType_SoulGem:
			return Settings::kMaxCountIngredient().i;
		case kFormType_Misc:
		case kFormType_Key:
			return Settings::kMaxCountMiscItem().i;
		case kFormType_AlchemyItem:
			return Settings::kMaxCountAlchemyItem().i;
		case kFormType_Ammo:
		case kFormType_Armor:
		case kFormType_Clothing:
		case kFormType_Weapon:
			return Settings::kMaxCountEquipment().i;
		default:
			return -1;
		}
	}

	ShadowCasterCountTable::ShadowCasterCountTable(UInt32 MaxSceneShadows)
	{
		MaxSceneShadowCount = MaxSceneShadows;
		ValidatedShadowCount = 0;

		for (int i = 0; i < kMaxShadows__MAX; i++)
			Current[i] = 0;
	}

	ShadowCasterCountTable::~ShadowCasterCountTable()
	{
		;//
	}

	bool ShadowCasterCountTable::ValidateCount( ShadowCaster* Caster )
	{
		SME_ASSERT(Caster);

		UInt8 Type = Caster->GetObject()->baseForm->typeID;
		UInt32* CurrentCount = GetCurrentCount(Type);
		SInt32 MaxCount = GetMaxCount(Type);

		if (MaxCount >= 0 && CurrentCount)
		{
			if ((*CurrentCount) + 1 > MaxCount)
				return false;
		}

		return true;
	}

	bool ShadowCasterCountTable::GetSceneSaturated( void ) const
	{
		if (ValidatedShadowCount >= MaxSceneShadowCount)
			return true;
		else
			return false;
	}

	void ShadowCasterCountTable::IncrementCount( ShadowCaster* Caster )
	{
		SME_ASSERT(Caster);

		UInt8 Type = Caster->GetObject()->baseForm->typeID;
		UInt32* CurrentCount = GetCurrentCount(Type);

		if (CurrentCount)
			(*CurrentCount)++;

		ValidatedShadowCount++;
	}

	ShadowCaster::ShadowCaster( NiNode* Node, TESObjectREFR* Object ) :
		Node(Node),
		Object(Object),
		Distance(0),
		BoundRadius(0),
		Actor(false),
		UnderWater(false)
	{
		SME_ASSERT(Node && Object && Object->baseForm && Object->parentCell);
		Distance = Utilities::GetDistanceFromPlayer(Node);
		BoundRadius = Node->m_kWorldBound.radius;
		Actor = Object->IsActor();

		if ((Object->parentCell->HasWater()))
		{
			ExtraWaterHeight* xWaterHeight = (ExtraWaterHeight*)Object->parentCell->extraData.GetByType(kExtraData_WaterHeight);
			if (xWaterHeight)
			{
				if (Node->m_worldTranslate.z < xWaterHeight->waterHeight)
					UnderWater = true;
			}
		}
	}

	ShadowCaster::~ShadowCaster()
	{
		;//
	}

	TESObjectREFR* ShadowCaster::GetObject( void ) const
	{
		return Object;
	}

	void ShadowCaster::GetDescription( std::string& Out ) const
	{
		char Buffer[0x200] = {0};
		FORMAT_STR(Buffer, "Caster %08X D[%f] BR[%f] [%s]", Object->refID, Distance, BoundRadius, Node->m_pcName);
		Out = Buffer;
	}

	bool ShadowCaster::Queue( ShadowSceneNode* Root, ShadowCasterCountTable* Count, ShadowSceneLight** OutSSL )
	{
		bool Result = false;

		if (OutSSL)
			*OutSSL = NULL;

		if (Count->ValidateCount(this) == false)
		{
			SHADOW_DEBUG(Object, "Failed Shadow Count check");
		}
		else if (!Settings::kExteriorDistanceCheck().i || Distance < Settings::kCasterMaxDistance().f)
		{
			if (Actor && Settings::kForceActorShadows().i)
			{
				ShadowSceneLight* SSL = CreateShadowSceneLight(Root);
				if (OutSSL)
					*OutSSL = SSL;

				SHADOW_DEBUG(Object, "Forced Actor shadow");
				return true;
			}

			if (BoundRadius >= Settings::kObjectTier1BoundRadius().f)
			{
				if (UnderWater == false)
				{
					BSFogProperty* Fog = TES::GetSingleton()->fogProperty;

					// don't queue if hidden by fog
					if (Fog == NULL || Distance < Fog->fogEnd || (Object->parentCell && Object->parentCell->IsInterior()))
					{
						if (Actor)
						{
							TESObjectREFR* Horse = thisVirtualCall<TESObjectREFR*>(0x380, Object);
							UInt32 Refraction = thisCall<UInt32>(0x005E9670, Object);
							UInt32 Invisibility = thisVirtualCall<UInt32>(0x284, Object, kActorVal_Invisibility);
							UInt32 SleepingState = thisVirtualCall<UInt32>(0x18C, Object);
							TESCreature* Creature = OBLIVION_CAST(Object->baseForm, TESForm, TESCreature);
							bool CreatureCheck = (Creature == NULL || (Creature->actorBaseData.flags & TESActorBaseData::kCreatureFlag_NoShadow) == false);

							if (Horse == NULL &&		// when not on horseback
								Refraction == 0 &&		// zero refraction
								Invisibility == 0 &&	// zero invisibility
								SleepingState != 4 &&	// not sitting/sleeping
								CreatureCheck)			// creature has shadow
							{
								Result = true;
							}
							else SHADOW_DEBUG(Object, "Failed Actor checks");
						}
						else
						{
							if (MainShadowExParams::Instance.GetAllowed(Node, Object))
								Result = true;
							else SHADOW_DEBUG(Object, "Failed MainShadowExParams check");
						}
					}
					else SHADOW_DEBUG(Object, "Failed Fog check (%f > %f)", Distance, Fog->fogEnd);
				}
				else SHADOW_DEBUG(Object, "Failed Underwater check");
			}
			else SHADOW_DEBUG(Object, "Failed Bound Radius check (%f)", BoundRadius);
		}
		else SHADOW_DEBUG(Object, "Failed Distance check (%f)", Distance);
		
		if (Result)
		{
			Result = ShadowRenderTasks::HasPlayerLOS(Object, Node, Distance);
			if (Result == false)
				SHADOW_DEBUG(Object, "Failed Player LOS check");
		}
		
		if (Result && Object->parentCell->IsInterior())
		{
			Result = ShadowRenderTasks::RunInteriorHeuristicGauntlet(Object, Node, BoundRadius);
			if (Result == false)
				SHADOW_DEBUG(Object, "Failed Interior Heuristic check");
		}

		ShadowSceneLight* Existing = Utilities::GetShadowCasterLight(Node);
		if (Result)
		{
			if (Existing)
			{
				Result = ShadowRenderTasks::PerformAuxiliaryChecks(Existing);

				// queue for forced light projection update
				if (Result == false)
					ShadowRenderTasks::LightProjectionUpdateQueue.push_back(Existing);
			}
		}

		if (Result)
		{
			ShadowSceneLight* SSL = CreateShadowSceneLight(Root);
			if (OutSSL)
				*OutSSL = SSL;

			Count->IncrementCount(this);

			SHADOW_DEBUG(Object, "Added to Shadow queue");
		}
		else SHADOW_DEBUG(Object, "Failed to queue");

		return Result;
	}

	ShadowSceneLight* ShadowCaster::CreateShadowSceneLight( ShadowSceneNode* Root )
	{
		Utilities::UpdateBounds(Node);
		thisCall<void>(0x007C6C30, Root, Node);

		ShadowSceneLight* ThisLight = NULL;
		for (NiTPointerList<ShadowSceneLight>::Node* Itr = Root->shadowCasters.start; Itr && Itr->data; Itr = Itr->next)
		{
			ShadowSceneLight* ShadowLight = Itr->data;
			if (ShadowLight->sourceNode == Node)
			{
				ThisLight = ShadowLight;
				break;
			}
		}

		SME_ASSERT(ThisLight);
		return ThisLight;
	}

	bool ShadowCaster::GetIsLargeObject( void ) const
	{
		return ShadowRenderTasks::IsLargeObject(Node);
	}

	bool ShadowCaster::GetIsMediumObject(void) const
	{
		return ShadowRenderTasks::IsMediumObject(Node);
	}

	bool ShadowCaster::SortComparatorDistance( ShadowCaster& LHS, ShadowCaster& RHS )
	{
		return LHS.Distance < RHS.Distance;
	}

	bool ShadowCaster::SortComparatorBoundRadius( ShadowCaster& LHS, ShadowCaster& RHS )
	{
		return LHS.BoundRadius > RHS.BoundRadius;
	}

	bool ShadowCaster::SortComparatorDistanceWeightedBoundRadius(ShadowCaster& LHS, ShadowCaster& RHS)
	{		
		return LHS.BoundRadius - LHS.Distance * Settings::kLargeObjectDistanceWeight().f > RHS.BoundRadius - RHS.Distance * Settings::kLargeObjectDistanceWeight().f;
	}

	bool ShadowCaster::SortComparatorBoundRadiusWeightedDistance(ShadowCaster& LHS, ShadowCaster& RHS)
	{			
		return LHS.Distance - LHS.BoundRadius * Settings::kBoundRadiusWeight().f < RHS.Distance - RHS.BoundRadius * Settings::kBoundRadiusWeight().f;
	}

	ShadowSceneProc::ShadowSceneProc( ShadowSceneNode* Root ) :
		Casters(),		
		Root(Root),
		ReducedGrids(Settings::kReduceGrids().i ? 1 : 0)
	{
		SME_ASSERT(Root && Root->m_children.data[3]);
	}

	ShadowSceneProc::~ShadowSceneProc()
	{
		Casters.clear();
	}

	void ShadowSceneProc::ProcessCell( TESObjectCELL* Cell )
	{
		SME_ASSERT(Cell);

		for (TESObjectCELL::ObjectListEntry* Itr = &Cell->objectList; Itr && Itr->refr; Itr = Itr->next)
		{
			TESObjectREFR* Object = Itr->refr;
			SME_ASSERT(Object->baseForm);

			if (Object->niNode && Object->refID != 0x14)
			{
				NiNode* Node = Object->niNode;
				BSFadeNode* FadeNode = NI_CAST(Node, BSFadeNode);

				if (FadeNode)
				{
					if (Object->baseForm->typeID != kFormType_Tree)
					{
						if (Node->m_kWorldBound.radius > 0.f)
						{
							if ((Object->flags & kTESFormSpecialFlag_DoesntCastShadow) == false)
							{
								// we allocate a BSXFlags extra data at instantiation
								if (BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kDontCastShadow) == false)
								{
									if (ShadowSundries::kExclusiveCaster == NULL || Object == ShadowSundries::kExclusiveCaster)
									{
										Casters.push_back(ShadowCaster(Node, Object));
										SHADOW_DEBUG(Object, "Added to Scene Caster List");
									}
									else SHADOW_DEBUG(Object, "Failed Exclusive Caster check");
								}
								else SHADOW_DEBUG(Object, "Failed BSXFlag DoesntCastShadow check");
							}
							else SHADOW_DEBUG(Object, "Failed TESForm DoesntCastShadow check");
						}
						else SHADOW_DEBUG(Object, "Failed Non-Zero Bounds check");
					}
					else SHADOW_DEBUG(Object, "Failed Tree Object check");
				}
			}
		}
	}

	void ShadowSceneProc::EnumerateSceneCasters( void )
	{
		Casters.push_back(ShadowCaster(Utilities::GetPlayerNode(), *g_thePlayer));

		// we'll walk the exterior cell grid/current interior cell
		if (TES::GetSingleton()->currentInteriorCell)
			ProcessCell(TES::GetSingleton()->currentInteriorCell);
		else
		{
			GridCellArray* CellGrid = TES::GetSingleton()->gridCellArray;			

			for (int i = ReducedGrids; i < CellGrid->size - ReducedGrids; i++)
			{
				for (int j = ReducedGrids; j < CellGrid->size - ReducedGrids; j++)
				{
					GridCellArray::GridEntry* Data = CellGrid->GetGridEntry(i, j);
					if (Data && Data->cell)
						ProcessCell(Data->cell);
				}
			}
			
		}
	}

	void ShadowSceneProc::Execute( UInt32 MaxShadowCount )
	{
		std::string Buffer;
		ShadowLightListT ValidSSLs;
		ShadowCasterCountTable CasterCount(MaxShadowCount);

		int TotalCount = 0;
		int TotalCountMax = Settings::kMaxCountTotalObject().i > -1 ? Settings::kMaxCountTotalObject().i : MaxShadowCount;

		Casters.clear();
		Casters.reserve(MaxShadowCount);

		if (Settings::kEnableDetailedDebugSelection().i)
		{
			_MESSAGE("Executing ShadowSceneProc...");
		}

		gLog.Indent();

		EnumerateSceneCasters();
		
		if (Settings::kLargeObjectHigherPriority().i)
		{
			CasterListT LargeCasters;

			// sort objects by bound radius descending 
			std::sort(Casters.begin(), Casters.end(), ShadowCaster::SortComparatorBoundRadius);

			// get all the large objects first
			int LargeCount = 0;
			int LargeCountMax = Settings::kMaxCountLargeObject().i > -1 ? Settings::kMaxCountLargeObject().i : TotalCountMax;			

			for (CasterListT::iterator Itr = Casters.begin(); Itr != Casters.end();)
			{				
				if (Itr->GetIsLargeObject() == false)
					break;

				if (ShadowRenderTasks::CanBeLargeObject(Itr->Node))
				{
					LargeCasters.push_back(*Itr);					
				}				

				// remove from regular casters list
				Itr = Casters.erase(Itr);				
			}
			
			// sort them by bound radius weighted by distance
			if (LargeCountMax > 1) {
				std::sort(LargeCasters.begin(), LargeCasters.end(), ShadowCaster::SortComparatorDistanceWeightedBoundRadius);
			}

			// queue them
			for (CasterListT::iterator Itr = LargeCasters.begin(); Itr != LargeCasters.end();)
			{			
				if (LargeCount >= LargeCountMax || CasterCount.GetSceneSaturated())
				{
					break;
				}

				ShadowSceneLight* NewSSL = NULL;
				if (Itr->Queue(Root, &CasterCount, &NewSSL) == true)
				{
					ValidSSLs.push_back(NewSSL);
					LargeCount++;

					if (Settings::kEnableDetailedDebugSelection().i)
					{
						Itr->GetDescription(Buffer);
						_MESSAGE("%s (Large Object) queued", Buffer.c_str());
					}
				}
				
				Itr++;
			}

			TotalCount = LargeCount;
		}
				
		// sort by least distance weighted by bound radius next
		std::sort(Casters.begin(), Casters.end(), ShadowCaster::SortComparatorBoundRadiusWeightedDistance);

		// now come the actors
		if (Settings::kForceActorShadows().i || (CasterCount.GetSceneSaturated() == false && TotalCount < TotalCountMax && Settings::kPrioritizeActorShadows().i))
		{
			int ActorCount = 0;

			for (CasterListT::iterator Itr = Casters.begin(); Itr != Casters.end();)
			{
				ShadowSceneLight* NewSSL = NULL;
				if (Itr->Actor)
				{
					if (Itr->Queue(Root, &CasterCount, &NewSSL) == true)
					{
						ValidSSLs.push_back(NewSSL);
						ActorCount++;

						if (Settings::kEnableDetailedDebugSelection().i)
						{
							Itr->GetDescription(Buffer);
	   					    _MESSAGE("%s (Actor) queued", Buffer.c_str());
						}
					}

					Itr = Casters.erase(Itr);
					continue;
				}

				if (Settings::kForceActorShadows().i == 0 && (CasterCount.GetSceneSaturated() || ActorCount >= Settings::kPrioritizeActorShadows().i || TotalCount >= TotalCountMax))
					break;

				Itr++;
			}

			TotalCount += ActorCount;
		}

		// the rest follow
		if (TotalCount < TotalCountMax && CasterCount.GetSceneSaturated() == false)
		{
			for (CasterListT::iterator Itr = Casters.begin(); Itr != Casters.end(); Itr++)
			{
				ShadowSceneLight* NewSSL = NULL;
				if (Itr->Queue(Root, &CasterCount, &NewSSL) == true)
				{
					ValidSSLs.push_back(NewSSL);
					TotalCount++;

					if (Settings::kEnableDetailedDebugSelection().i)
					{
						Itr->GetDescription(Buffer);
						_MESSAGE("%s queued", Buffer.c_str());
					}
				}

				if (TotalCount >= TotalCountMax || CasterCount.GetSceneSaturated())
					break;
			}
		}
		
		gLog.Outdent();
	}

	void ShadowSceneProc::DebugDump( void ) const
	{
		_MESSAGE("Dumping ShadowSceneProc...");
		gLog.Indent();

		std::string Desc;
		for (CasterListT::const_iterator Itr = Casters.begin(); Itr != Casters.end(); Itr++)
		{
			Itr->GetDescription(Desc);
			_MESSAGE("%s", Desc.c_str());
		}

		gLog.Outdent();
	}

	ShadowExclusionParameters::~ShadowExclusionParameters()
	{
		;//
	}

	void ShadowExclusionParameters::LoadParameters( UInt8 ParamType, SME::INI::INISetting* ExcludedTypes, SME::INI::INISetting* ExcludedPaths )
	{
		SME_ASSERT(ParamType < kParamType__MAX);

		ParameterData* DataStore = &Parameters[ParamType];

		switch (ParamType)
		{
		case kParamType_Interior:
			_MESSAGE("Interior:");
			break;
		case kParamType_Exterior:
			_MESSAGE("Exterior:");
			break;
		}

		DataStore->PathsSource = ExcludedPaths;
		DataStore->TypesSource = ExcludedTypes;
		DataStore->Refresh();

		gLog.Indent();

		_MESSAGE("Object types:");
		DataStore->ObjectTypes.Dump();

		_MESSAGE("Path strings:");
		DataStore->PathSubstrings.Dump();

		gLog.Outdent();
	}

	bool ShadowExclusionParameters::GetAllowed( NiNode* Node, TESObjectREFR* Object ) const
	{
		bool Result = true;

		SME_ASSERT(Node && Object);

		TESObjectCELL* ParentCell = Object->parentCell;
		BSXFlags* xFlags = Utilities::GetBSXFlags(Node);
		SME_ASSERT(ParentCell && xFlags);

		if (ParentCell->IsInterior() == true && GetAllowedInterior(Node, xFlags) == false)
			Result = false;
		else if (ParentCell->IsInterior() == false && GetAllowedExterior(Node, xFlags) == false)
			Result = false;

		if (Result)
		{
			const ParameterData* DataStore = NULL;

			if (ParentCell->IsInterior())
				DataStore = &Parameters[kParamType_Interior];
			else
				DataStore = &Parameters[kParamType_Exterior];

			if (std::find(DataStore->ObjectTypes().begin(), DataStore->ObjectTypes().end(), Object->baseForm->typeID) != DataStore->ObjectTypes().end())
				Result = false;
		}

		return Result;
	}

	void ShadowExclusionParameters::HandleModelLoad( NiNode* Node, BSXFlags* xFlags ) const
	{
		SME_ASSERT(Node);

		std::string NodeName("");
		if (Node->m_pcName)
			NodeName = Node->m_pcName;

		if (NodeName.length())
		{
			SME::StringHelpers::MakeLower(NodeName);
			if (NodeName.find("base") == 0)
			{
				gLog.Indent();

				SetInteriorFlag(false, Node, xFlags);
				SetExteriorFlag(false, Node, xFlags);

				for (PathSubstringListT::ParameterListT::const_iterator Itr = Parameters[kParamType_Interior].PathSubstrings().begin();
					Itr != Parameters[kParamType_Interior].PathSubstrings().end();
					Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						SetInteriorFlag(true, Node, xFlags);
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = Parameters[kParamType_Exterior].PathSubstrings().begin();
					Itr != Parameters[kParamType_Exterior].PathSubstrings().end();
					Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						SetExteriorFlag(true, Node, xFlags);
						break;
					}
				}

				gLog.Outdent();
			}
		}
	}

	void ShadowExclusionParameters::RefreshParameters( void )
	{
		Parameters[kParamType_Interior].Refresh();
		Parameters[kParamType_Exterior].Refresh();
	}

	MainShadowExParams		MainShadowExParams::Instance;

	void MainShadowExParams::SetInteriorFlag( bool State, NiNode* Node, BSXFlags* xFlags ) const
	{
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontCastInteriorShadow, State);
	}

	void MainShadowExParams::SetExteriorFlag( bool State, NiNode* Node, BSXFlags* xFlags ) const
	{
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontCastExteriorShadow, State);
	}

	bool MainShadowExParams::GetAllowedInterior( NiNode* Node, BSXFlags* xFlags ) const
	{
		return NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontCastInteriorShadow) == false;
	}

	bool MainShadowExParams::GetAllowedExterior( NiNode* Node, BSXFlags* xFlags ) const
	{
		return NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontCastExteriorShadow) == false;
	}

	const char* MainShadowExParams::GetDescription( void ) const
	{
		return "MainShadow";
	}

	MainShadowExParams::~MainShadowExParams()
	{
		;//
	}

	void MainShadowExParams::Initialize( void )
	{
		_MESSAGE("Loading %s exclusion params...", GetDescription());
		gLog.Indent();

		LoadParameters(kParamType_Interior, &Settings::kMainExcludedTypesInterior, &Settings::kMainExcludedPathInterior);
		LoadParameters(kParamType_Exterior, &Settings::kMainExcludedTypesExterior, &Settings::kMainExcludedPathExterior);

		gLog.Outdent();
	}

	SelfShadowExParams			SelfShadowExParams::Instance;

	void SelfShadowExParams::SetInteriorFlag( bool State, NiNode* Node, BSXFlags* xFlags ) const
	{
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontCastInteriorSelfShadow, State);
	}

	void SelfShadowExParams::SetExteriorFlag( bool State, NiNode* Node, BSXFlags* xFlags ) const
	{
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontCastExteriorSelfShadow, State);
	}

	bool SelfShadowExParams::GetAllowedInterior( NiNode* Node, BSXFlags* xFlags ) const
	{
		return NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontCastInteriorSelfShadow) == false;
	}

	bool SelfShadowExParams::GetAllowedExterior( NiNode* Node, BSXFlags* xFlags ) const
	{
		return NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontCastExteriorSelfShadow) == false;
	}

	const char* SelfShadowExParams::GetDescription( void ) const
	{
		return "SelfShadow";
	}

	SelfShadowExParams::~SelfShadowExParams()
	{
		;//
	}

	void SelfShadowExParams::Initialize( void )
	{
		_MESSAGE("Loading %s exclusion params...", GetDescription());
		gLog.Indent();

		LoadParameters(kParamType_Interior, &Settings::kSelfExcludedTypesInterior, &Settings::kSelfExcludedPathInterior);
		LoadParameters(kParamType_Exterior, &Settings::kSelfExcludedTypesExterior, &Settings::kSelfExcludedPathExterior);

		gLog.Outdent();
	}

	ShadowReceiverExParams			ShadowReceiverExParams::Instance;

	void ShadowReceiverExParams::SetInteriorFlag( bool State, NiNode* Node, BSXFlags* xFlags ) const
	{
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontReceiveInteriorShadow, State);
	}

	void ShadowReceiverExParams::SetExteriorFlag( bool State, NiNode* Node, BSXFlags* xFlags ) const
	{
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontReceiveExteriorShadow, State);
	}

	bool ShadowReceiverExParams::GetAllowedInterior( NiNode* Node, BSXFlags* xFlags ) const
	{
		return NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontReceiveInteriorShadow) == false;
	}

	bool ShadowReceiverExParams::GetAllowedExterior( NiNode* Node, BSXFlags* xFlags ) const
	{
		return NiAVObjectSpecialFlags::GetFlag(Node, NiAVObjectSpecialFlags::kDontReceiveExteriorShadow) == false;
	}

	const char* ShadowReceiverExParams::GetDescription( void ) const
	{
		return "ShadowReceiver";
	}

	ShadowReceiverExParams::~ShadowReceiverExParams()
	{
		;//
	}

	void ShadowReceiverExParams::Initialize( void )
	{
		_MESSAGE("Loading %s exclusion params...", GetDescription());
		gLog.Indent();

		LoadParameters(kParamType_Interior, &Settings::kReceiverExcludedTypesInterior, &Settings::kReceiverExcludedPathInterior);
		LoadParameters(kParamType_Exterior, &Settings::kReceiverExcludedTypesExterior, &Settings::kReceiverExcludedPathExterior);

		gLog.Outdent();
	}

	ShadowReceiverValidator::ShadowReceiverValidator( NiNodeListT* OutList ) :
		NonReceivers(OutList)
	{
		SME_ASSERT(OutList);
	}

	ShadowReceiverValidator::~ShadowReceiverValidator()
	{
		;//
	}

	bool ShadowReceiverValidator::AcceptBranch( NiNode* Node )
	{
		BSFadeNode* FadeNode = NI_CAST(Node, BSFadeNode);
		BSTreeNode* TreeNode = NI_CAST(Node, BSTreeNode);

		if (FadeNode)
		{
			TESObjectREFR* Object = Utilities::GetNodeObjectRef(FadeNode);
			if (FadeNode->IsCulled() == false && Object && Object->parentCell && ShadowRenderTasks::CanReceiveShadow(FadeNode) == false)
			{
				SHADOW_DEBUG(Object, "Queued for Shadow Receiver culling");
				NonReceivers->push_back(FadeNode);
			}

			return false;
		}
		else if (TreeNode && TreeNode->IsCulled() == false)
		{
			// we don't like trees...
			NonReceivers->push_back(TreeNode);
			return false;
		}

		return true;
	}

	void ShadowReceiverValidator::AcceptLeaf( NiAVObject* Object )
	{
		;//
	}

	FadeNodeShadowFlagUpdater::~FadeNodeShadowFlagUpdater()
	{
		;//
	}

	bool FadeNodeShadowFlagUpdater::AcceptBranch( NiNode* Node )
	{
		BSFadeNode* FadeNode = NI_CAST(Node, BSFadeNode);
		if (FadeNode)
		{
			ShadowRenderTasks::HandleModelLoad(FadeNode, false);
			return false;
		}

		return true;
	}

	void FadeNodeShadowFlagUpdater::AcceptLeaf( NiAVObject* Object )
	{
		;//
	}

	bool NiAVObjectSpecialFlags::GetFlag( NiAVObject* Node, UInt16 Flag )
	{
		SME_ASSERT(Node && Flag > k__BEGININTERNAL);

		return (Node->m_flags & Flag) != 0;
	}

	void NiAVObjectSpecialFlags::SetFlag( NiAVObject* Node, UInt16 Flag, bool State )
	{
		SME_ASSERT(Node && Flag > k__BEGININTERNAL);

		if (State)
			Node->m_flags |= Flag;
		else
			Node->m_flags &= ~Flag;
	}

	bool BSXFlagsSpecialFlags::GetFlag( BSXFlags* Store, UInt32 Flag )
	{
		SME_ASSERT(Store && Flag > k__BEGININTERNAL);

		return (Store->m_iValue & Flag) != 0;
	}

	void BSXFlagsSpecialFlags::SetFlag( BSXFlags* Store, UInt32 Flag, bool State )
	{
		SME_ASSERT(Store && Flag > k__BEGININTERNAL && Flag < k__BEGINEXTERNAL);

		// can only set internal flags
		if (State)
			Store->m_iValue |= Flag;
		else
			Store->m_iValue &= ~Flag;
	}

	bool BSXFlagsSpecialFlags::GetFlag( NiAVObject* Node, UInt32 Flag )
	{
		SME_ASSERT(Node);

		return GetFlag(Utilities::GetBSXFlags(Node), Flag);
	}

	void BSXFlagsSpecialFlags::SetFlag( NiAVObject* Node, UInt32 Flag, bool State )
	{
		SME_ASSERT(Node);

		return SetFlag(Utilities::GetBSXFlags(Node), Flag, State);
	}

	ShadowLightListT			ShadowRenderTasks::LightProjectionUpdateQueue;
	const float					ShadowRenderTasks::ShadowDepthBias = -0.000001;
	PathSubstringListT			ShadowRenderTasks::BackFaceIncludePaths;
	PathSubstringListT			ShadowRenderTasks::LargeObjectExcludePaths;
	PathSubstringListT			ShadowRenderTasks::LightLOSCheckExcludePaths;
	PathSubstringListT			ShadowRenderTasks::InteriorHeuristicsIncludePaths;
	PathSubstringListT			ShadowRenderTasks::InteriorHeuristicsExcludePaths;
	const float					ShadowRenderTasks::DirectionalLightCheckThresholdDistance = 50.f;
	PathSubstringListT			ShadowRenderTasks::SelfExclusiveIncludePathsExterior;
	PathSubstringListT			ShadowRenderTasks::SelfExclusiveIncludePathsInterior;

	void ShadowRenderTasks::ToggleBackFaceCulling(bool State)
	{
		if (State)
			(*g_renderer)->device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		else
			(*g_renderer)->device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	}

	void ShadowRenderTasks::PerformModelLoadTask(NiNode* Node, BSXFlags* xFlags)
	{
		SME_ASSERT(Node);

		std::string NodeName("");
		if (Node->m_pcName)
			NodeName = Node->m_pcName;

		if (NodeName.length())
		{
			SME::StringHelpers::MakeLower(NodeName);
			if (NodeName.find("base") == 0)
			{
				BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kCannotBeLargeObject, false);
				BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kRenderBackFacesToShadowMap, false);
				BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kDontPerformLOSCheck, false);
				BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kAllowInteriorHeuristics, false);
				BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kOnlySelfShadowInterior, false);
				BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kOnlySelfShadowExterior, false);

				for (PathSubstringListT::ParameterListT::const_iterator Itr = BackFaceIncludePaths().begin(); Itr != BackFaceIncludePaths().end(); Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kRenderBackFacesToShadowMap, true);
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = LargeObjectExcludePaths().begin(); Itr != LargeObjectExcludePaths().end(); Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kCannotBeLargeObject, true);
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = LightLOSCheckExcludePaths().begin(); Itr != LightLOSCheckExcludePaths().end(); Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kDontPerformLOSCheck, true);
						break;
					}
				}

				if (InteriorHeuristicsIncludePaths().size() == 0)
					BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kAllowInteriorHeuristics, true);
				else for (PathSubstringListT::ParameterListT::const_iterator Itr = InteriorHeuristicsIncludePaths().begin();
																			Itr != InteriorHeuristicsIncludePaths().end();
																			Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kAllowInteriorHeuristics, true);
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = InteriorHeuristicsExcludePaths().begin();
																			Itr != InteriorHeuristicsExcludePaths().end();
																			Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						// disallow
						BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kAllowInteriorHeuristics, false);
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = SelfExclusiveIncludePathsInterior().begin();
																		Itr != SelfExclusiveIncludePathsInterior().end();
																		Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kOnlySelfShadowInterior, true);
						break;
					}
				}

				for (PathSubstringListT::ParameterListT::const_iterator Itr = SelfExclusiveIncludePathsExterior().begin();
																		Itr != SelfExclusiveIncludePathsExterior().end();
																		Itr++)
				{
					if (NodeName.find(*Itr) != std::string::npos)
					{
						BSXFlagsSpecialFlags::SetFlag(xFlags, BSXFlagsSpecialFlags::kOnlySelfShadowExterior, true);
						break;
					}
				}
			}
		}
	}

	void ShadowRenderTasks::HandleMainProlog( void )
	{
		ShadowFigures::ShadowRenderConstantRegistry::GetSingleton()->UpdateConstants();

		if (ShadowSundries::kDebugSelection && Utilities::GetConsoleOpen() == false)
		{
			_MESSAGE("============================= BEGIN SHADOW RENDER MAIN ROUTINE =============================================");
		}
	}

	void ShadowRenderTasks::HandleMainEpilog( void )
	{
		if (ShadowSundries::kDebugSelection && Utilities::GetConsoleOpen() == false)
		{
			_MESSAGE("============================= END SHADOW RENDER MAIN ROUTINE ===============================================\n");
		}
	}

	void __stdcall ShadowRenderTasks::QueueShadowOccluders(UInt32 MaxShadowCount)
	{
		if (InterfaceManager::GetSingleton()->IsGameMode() == false)
			return;

		TESWeather* CurrentWeather = Sky::GetSingleton()->firstWeather;
		if (CurrentWeather && TES::GetSingleton()->currentInteriorCell == NULL)
		{
			UInt8 WeatherType = Utilities::GetWeatherClassification(CurrentWeather);
			if (WeatherType == TESWeather::kType_Cloudy && Settings::kWeatherDisableCloudy().i)
				return;
			else if (WeatherType == TESWeather::kType_Rainy && Settings::kWeatherDisableRainy().i)
				return;
			else if (WeatherType == TESWeather::kType_Snow && Settings::kWeatherDisableSnow().i)
				return;
		}

		ShadowSceneNode* RootNode = Utilities::GetShadowSceneNode();
		if (RootNode)
		{
			ShadowSceneProc SceneProc(RootNode);
			SceneProc.Execute(MaxShadowCount);
		}
	}

	bool __stdcall ShadowRenderTasks::HandleSelfShadowing( ShadowSceneLight* Caster )
	{
		SME_ASSERT(Caster);

		NiNode* Node = Caster->sourceNode;
		TESObjectREFR* Object = Utilities::GetNodeObjectRef(Node);
		bool Result = false;

		if (Object && Object->parentCell)
		{
			if (SelfShadowExParams::Instance.GetAllowed(Node, Object))
				Result = true;
			else SHADOW_DEBUG(Object, "Failed SelfShadowExParams check");
		}
		else
			Result = true;		// projectiles are an exception

		return Result;
	}

	void __stdcall ShadowRenderTasks::HandleModelLoad( NiNode* Node, bool Allocation )
	{
		// add BSXFlags if necessary
		BSXFlags* xFlags = Utilities::GetBSXFlags(Node, Allocation);

		MainShadowExParams::Instance.HandleModelLoad(Node, xFlags);
		SelfShadowExParams::Instance.HandleModelLoad(Node, xFlags);
		ShadowReceiverExParams::Instance.HandleModelLoad(Node, xFlags);
		PerformModelLoadTask(Node, xFlags);
	}

	void __stdcall ShadowRenderTasks::HandleShadowMapRenderingProlog( NiNode* Node, ShadowSceneLight* Source )
	{
		if (BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kRenderBackFacesToShadowMap))
		{
			ToggleBackFaceCulling(false);
		}
	}

	void __stdcall ShadowRenderTasks::HandleShadowMapRenderingEpilog( NiNode* Node, ShadowSceneLight* Source )
	{
		if (BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kRenderBackFacesToShadowMap))
		{
			ToggleBackFaceCulling(true);
		}

		if (Settings::kEnableDebugShader().i)
		{
			if (ShadowSundries::kDebugSelection == NULL || ShadowSundries::kDebugSelection == Utilities::GetNodeObjectRef(Node))
				Source->showDebug = 1;
			else
				Source->showDebug = 0;
		}
		else
			Source->showDebug = 0;
	}

	bool ShadowRenderTasks::CanBeLargeObject( NiNode* Node )
	{
		SME_ASSERT(Node);

		return BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kCannotBeLargeObject) == false;
	}

	bool ShadowRenderTasks::IsLargeObject( NiNode* Node )
	{
		SME_ASSERT(Node);

		if (Node->m_kWorldBound.radius >= Settings::kObjectTier6BoundRadius().f)
			return true;
		else
			return false;
	}

	bool ShadowRenderTasks::IsMediumObject(NiNode* Node)
	{
		SME_ASSERT(Node);

		if (Node->m_kWorldBound.radius >= Settings::kObjectTier5BoundRadius().f && Node->m_kWorldBound.radius < Settings::kObjectTier6BoundRadius().f)
			return true;
		else
			return false;
	}

	void ShadowRenderTasks::Initialize( void )
	{
		_MESSAGE("Loading backface rendering whitelist...");
		BackFaceIncludePaths.Refresh(&Settings::kRenderBackfacesIncludePath);
		BackFaceIncludePaths.Dump();

		_MESSAGE("Loading large object blacklist...");
		LargeObjectExcludePaths.Refresh(&Settings::kLargeObjectExcludedPath);
		LargeObjectExcludePaths.Dump();

		_MESSAGE("Loading light LOS check blacklist...");
		LightLOSCheckExcludePaths.Refresh(&Settings::kLightLOSExcludedPath);
		LightLOSCheckExcludePaths.Dump();

		_MESSAGE("Loading interior heuristic whitelist...");
		InteriorHeuristicsIncludePaths.Refresh(&Settings::kInteriorHeuristicsIncludePath);
		InteriorHeuristicsIncludePaths.Dump();

		_MESSAGE("Loading interior heuristic blacklist...");
		InteriorHeuristicsExcludePaths.Refresh(&Settings::kInteriorHeuristicsExcludePath);
		InteriorHeuristicsExcludePaths.Dump();

		_MESSAGE("Loading interior self-shadow-only whitelist...");
		SelfExclusiveIncludePathsInterior.Refresh(&Settings::kSelfIncludePathInterior);
		SelfExclusiveIncludePathsInterior.Dump();

		_MESSAGE("Loading exterior self-shadow-only whitelist...");
		SelfExclusiveIncludePathsExterior.Refresh(&Settings::kSelfIncludePathExterior);
		SelfExclusiveIncludePathsExterior.Dump();
	}

	void ShadowRenderTasks::RefreshMiscPathLists( void )
	{
		BackFaceIncludePaths.Refresh(&Settings::kRenderBackfacesIncludePath);
		LargeObjectExcludePaths.Refresh(&Settings::kLargeObjectExcludedPath);
		LightLOSCheckExcludePaths.Refresh(&Settings::kLightLOSExcludedPath);
		InteriorHeuristicsIncludePaths.Refresh(&Settings::kInteriorHeuristicsIncludePath);
		InteriorHeuristicsExcludePaths.Refresh(&Settings::kInteriorHeuristicsExcludePath);
		SelfExclusiveIncludePathsInterior.Refresh(&Settings::kSelfIncludePathInterior);
		SelfExclusiveIncludePathsExterior.Refresh(&Settings::kSelfIncludePathExterior);
	}

	bool __stdcall ShadowRenderTasks::PerformAuxiliaryChecks(ShadowSceneLight* Source)
	{
		SME_ASSERT(Source);

		bool Result = true;

		NiNode* Node = Source->sourceNode;
		NiLight* Light = Source->sourceLight;

		if (Light && Node && InterfaceManager::GetSingleton()->IsGameMode())
		{
			TESObjectREFR* Object = Utilities::GetNodeObjectRef(Source->sourceNode);
			if (Object)
			{
				UInt8 SelfShadowsState = *(UInt8*)0x00B06F0C;
				if (SelfShadowsState == 0 && PerformExclusiveSelfShadowCheck(Node, Object) == false)
				{
					// preemptively cull the caster if it's self shadow only and the option is turned off
					Result = false;
				}

				if (Result)
				{
					Result = PerformShadowLightSourceCheck(Source, Object);
					if (Result)
					{
						SHADOW_DEBUG(Object, "Light LOS/Direction check (L[%f, %f, %f] ==> DIST[%f])",
									 Source->sourceLight->m_worldTranslate.x,
									 Source->sourceLight->m_worldTranslate.y,
									 Source->sourceLight->m_worldTranslate.z,
									 Utilities::GetDistance(Source->sourceLight, Node));
						gLog.Indent();
						Result = PerformLightLOSCheck(Source, Object);
						gLog.Outdent();
					}
				}
				else SHADOW_DEBUG(Object, "Failed Prelim Exclusive Self-Shadows check");
			}
		}

		return Result;
	}

	void __stdcall ShadowRenderTasks::HandleShadowLightUpdateReceiver( ShadowSceneLight* Source, NiNode* SceneGraph )
	{
		SME_ASSERT(Source && SceneGraph);

		bool AllowReceiver = true;
		if (Source->sourceNode)
		{
			TESObjectREFR* Object = Utilities::GetNodeObjectRef(Source->sourceNode);
			if (Object)
			{
				if (PerformExclusiveSelfShadowCheck(Source->sourceNode, Object) == false)
				{
					AllowReceiver = false;
					SHADOW_DEBUG(Object, "Failed Final Exclusive Self-Shadows check");
				}
			}
		}

		if (AllowReceiver)
		{
			// prevents casters from self occluding regardless of the self shadow setting
			Source->sourceNode->SetCulled(true);

			thisCall<void>(0x007D6900, Source, SceneGraph);

			Source->sourceNode->SetCulled(false);
		}
	}

	bool __stdcall ShadowRenderTasks::ReactsToSmallLights( ShadowSceneLight* Source )
	{
		SME_ASSERT(Source);

		if (TES::GetSingleton()->currentInteriorCell == NULL && IsLargeObject(Source->sourceNode) && Settings::kLargeObjectSunShadowsOnly().i)
			return false;
		else
			return true;
	}

	bool ShadowRenderTasks::HasPlayerLOS( TESObjectREFR* Object, NiNode* Node, float Distance )
	{
		SME_ASSERT(Object && Node);

		if (Settings::kPlayerLOSExcludeLightLOS().i && BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kDontPerformLOSCheck) == true)
			return true;

		bool Interior = Object->parentCell->IsInterior();

		if (Object != *g_thePlayer &&
			((Interior && Settings::kPlayerLOSCheckInterior().i) ||
			(Interior == false && Object->parentCell == (*g_thePlayer)->parentCell && Settings::kPlayerLOSCheckExterior().i)))
		{
			bool BoundsCheck = (Interior || Node->m_kWorldBound.radius < Settings::kObjectTier5BoundRadius().f);
			if (BoundsCheck == true)
			{
				bool Above = Utilities::GetAbovePlayer(Object, 10);
				bool Below = Utilities::GetBelowPlayer(Object, 35);
				bool CloseToPC = Distance < Settings::kPlayerLOSCheckThresholdDist().f;
				bool HighAccuracy = Settings::kPlayerLOSCheckHighAccuracy().i != 0;

				if (Above == true || Below == true || HighAccuracy == true)
				{
					if ((Interior == false && CloseToPC == false) ||
						(Interior == true && (CloseToPC == false || Above == true || Below == true)))
					{
						return Utilities::GetPlayerHasLOS(Object, true);
					}
				}
			}
		}

		return true;
	}

	void __stdcall ShadowRenderTasks::HandleShadowReceiverLightingPropertyUpdate( ShadowSceneLight* Source, NiNode* Receiver )
	{
		SME_ASSERT(Source && Receiver);

		NiNode* FadeNode = NI_CAST(Receiver, BSFadeNode);
		if (FadeNode)
		{
			TESObjectREFR* Object = Utilities::GetNodeObjectRef(FadeNode);
			if (FadeNode->IsCulled() == false && (Settings::kReceiverEnableExclusionParams().i == 0 || CanReceiveShadow(FadeNode)))
			{
				thisCall<void>(0x007D59E0, Source, Receiver);
				SHADOW_DEBUG(Object, "Updating Geometry for Self Shadow");
			}

			return;
		}

		NiNodeListT NonReceivers;
		if (Settings::kReceiverEnableExclusionParams().i)
		{
			// walking the scenegraph doesn't come without overhead
			Utilities::NiNodeChildrenWalker Walker(Receiver);

			Walker.Walk(&ShadowReceiverValidator(&NonReceivers));

			for (NiNodeListT::iterator Itr = NonReceivers.begin(); Itr != NonReceivers.end(); Itr++)
				(*Itr)->SetCulled(true);
		}

		// uncull the player first person node to receive shadows
		bool UnCullFPNode = Settings::kActorsReceiveAllShadows().i &&
							Utilities::GetPlayerNode(true) &&
							Utilities::GetPlayerNode(true)->IsCulled() &&
							(*g_thePlayer)->IsThirdPerson() == false;

		if (UnCullFPNode)
			Utilities::GetPlayerNode(true)->SetCulled(false);

		thisCall<void>(0x007D59E0, Source, Receiver);

		if (UnCullFPNode)
			Utilities::GetPlayerNode(true)->SetCulled(true);

		if (Settings::kReceiverEnableExclusionParams().i)
		{
			for (NiNodeListT::iterator Itr = NonReceivers.begin(); Itr != NonReceivers.end(); Itr++)
				(*Itr)->SetCulled(false);
		}
	}

	bool ShadowRenderTasks::CanReceiveShadow( NiNode* Node )
	{
		SME_ASSERT(Node);

		bool Result = true;
		TESObjectREFR* Object = Utilities::GetNodeObjectRef(Node);
		if (Object && Object->parentCell)
		{
			Result = ShadowReceiverExParams::Instance.GetAllowed(Node, Object);
			if (Result)
			{
				if (BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kDontReceiveShadow))
				{
					Result = false;
					SHADOW_DEBUG(Object, "Failed BSXFlags DontReceiveShadow check");
				}
			}
			else SHADOW_DEBUG(Object, "Failed ShadowReceiverExParams check");
		}

		return Result;
	}

	bool ShadowRenderTasks::RunInteriorHeuristicGauntlet( TESObjectREFR* Caster, NiNode* Node, float BoundRadius )
	{
		bool Result = true;

		SME_ASSERT(Caster && Node);

		// only for static objects
		if (Caster->baseForm->typeID == kFormType_Stat)
		{
			if (BoundRadius > Settings::kObjectTier4BoundRadius().f)
			{
				if (BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kAllowInteriorHeuristics))
				{
					Result = false;
				}
			}
		}

		return Result;
	}

	bool __stdcall ShadowRenderTasks::CanHaveDirectionalShadow( ShadowSceneLight* Source )
	{
		SME_ASSERT(Source && Source->sourceNode);

		TESObjectREFR* Object = Utilities::GetNodeObjectRef(Source->sourceNode);

		if (Object && Object->parentCell->IsInterior() && Object->parentCell->BehavesLikeExterior() == false && Settings::kNoInteriorSunShadows().i)
		{
			float BoundRadius = Source->sourceNode->m_kWorldBound.radius;
			if (BoundRadius > Settings::kObjectTier3BoundRadius().f && Object->IsActor() == false)
			{
				// since the coords scale with the projection multiplier, small objects are excluded
				// also, we exclude actors due to their mobility
				return false;
			}
		}

		if (Object && Object->parentCell->IsInterior() == false && Settings::kNightTimeMoonShadows().i == 0)
		{
			if (TimeGlobals::GameHour() < 6.5 || TimeGlobals::GameHour() > 18.5)
				return false;
		}

		return true;
	}

	bool ShadowRenderTasks::PerformExclusiveSelfShadowCheck( NiNode* Node, TESObjectREFR* Object )
	{
		if ((BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kOnlySelfShadowInterior) && Object->parentCell->IsInterior()) ||
			(BSXFlagsSpecialFlags::GetFlag(Node, BSXFlagsSpecialFlags::kOnlySelfShadowExterior) && Object->parentCell->IsInterior() == false))
		{
			return false;
		}

		return true;
	}

	bool ShadowRenderTasks::PerformShadowLightSourceCheck( ShadowSceneLight* Source, TESObjectREFR* Object )
	{
		bool Result = true;

		if ((Source->unkFCPad[1] == kSSLExtraFlag_NoShadowLightSource))
		{
			Result = false;
			SHADOW_DEBUG(Object, "Failed Shadow Light Source check");
		}

		return Result;
	}

	bool ShadowRenderTasks::PerformLightLOSCheck( ShadowSceneLight* Source, TESObjectREFR* Object )
	{
		bool Result = true;

		if (Source->sourceLight->m_worldTranslate.x != 0 &&
			Source->sourceLight->m_worldTranslate.y != 0 &&
			Source->sourceLight->m_worldTranslate.z != 0 )
		{
			if (BSXFlagsSpecialFlags::GetFlag(Source->sourceNode, BSXFlagsSpecialFlags::kDontPerformLOSCheck) == false)
			{
				if (IsLargeObject(Source->sourceNode) == false || Settings::kLightLOSSkipLargeObjects().i == 0)
				{
					if (Object->IsActor() == false || Settings::kLightLOSSkipActors().i == 0)
					{
						// light LOS checks don't really work well in interiors as they're performed on the projected translation coords of the source light
						// only small objects, i.e, those that use a close-to-vanilla projection multiplier pass these checks in such cells
						// so we'll limit it to them (just as well, as we're only concerned about them anyway)
						bool CheckInterior = Object->parentCell->IsInterior() &&
							Settings::kLightLOSCheckInterior().i &&
							Source->sourceNode->m_kWorldBound.radius < Settings::kObjectTier2BoundRadius().f;

						if (CheckInterior || (Object->parentCell->IsInterior() == false && Settings::kLightLOSCheckExterior().i))
						{
							if (Utilities::GetDistanceFromPlayer(Source->sourceNode) < Settings::kCasterMaxDistance().f)
							{
								bool LOSCheck = Utilities::GetLightLOS(Source->sourceLight, Object);
								if (LOSCheck == false)
								{
									Result = false;
								}
								SHADOW_DEBUG(Object, "LOS[%d]", LOSCheck);
							}
						}
						else SHADOW_DEBUG(Object, "Skipped with Light LOS Cell/Bound Radius check");
					}
					else SHADOW_DEBUG(Object, "Skipped with Light LOS Actor check");
				}
				else SHADOW_DEBUG(Object, "Skipped with Light LOS Large Object check");
			}
			else SHADOW_DEBUG(Object, "Skipped BSXFlags DontPerformLOS check");
		}

		return Result;
	}

	void __stdcall ShadowRenderTasks::HandleTreeModelLoad( BSTreeNode* Node )
	{
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontReceiveInteriorShadow, true);
		NiAVObjectSpecialFlags::SetFlag(Node, NiAVObjectSpecialFlags::kDontReceiveExteriorShadow, true);
	}

	void __stdcall ShadowRenderTasks::HandleSSLCreation(ShadowSceneLight* Light)
	{
		SME_ASSERT(Light);

		// init padding bytes to store extra state
		Light->unkFCPad[0] = 0;			// stores extra flags
		Light->unkFCPad[1] = 0;			// set if culled due to no light source
		Light->unkFCPad[2] = 0;
	}

	void __stdcall ShadowRenderTasks::HandleLightProjectionProlog(ShadowSceneLight* Source)
	{
		SME_ASSERT(Source);

		// reset extra flags
		Source->unkFCPad[0] = 0;
		Source->unkFCPad[1] = 0;
	}

	void __stdcall ShadowRenderTasks::HandleLightProjectionEpilog(ShadowSceneLight* Source)
	{
		SME_ASSERT(Source);

		// check if the projection failed and cull if necessary
		UInt8 ExtraFlags = Source->unkFCPad[0];
		bool Cull = false;

		if ((ExtraFlags & kSSLExtraFlag_NoActiveLights) && (ExtraFlags & kSSLExtraFlag_DisallowDirectionalLight))
			Cull = true;
		else if ((ExtraFlags & kSSLExtraFlag_DisallowSmallLights) && (ExtraFlags & kSSLExtraFlag_DisallowDirectionalLight))
			Cull = true;

		if (Cull)
		{
			Source->unkFCPad[1] = kSSLExtraFlag_NoShadowLightSource;
			SHADOW_DEBUG(Utilities::GetNodeObjectRef(Source->sourceNode), "Failed Light Projection Calc");
		}
	}

	bool __stdcall ShadowRenderTasks::HandleLightProjectionStage1(ShadowSceneLight* Source, int ActiveLights)
	{
		// update extra flags and check if source reacts to small lights/has any active lights
		SME_ASSERT(Source);

		ShadowLightListT Lights;
		if (Utilities::GetNodeActiveLights(Source->sourceNode, &Lights, Utilities::ActiveShadowSceneLightEnumerator::kParam_NonShadowCasters) == 0)
			Source->unkFCPad[0] |= kSSLExtraFlag_NoActiveLights;

		bool Result = ReactsToSmallLights(Source);

		if (Result == false)
			Source->unkFCPad[0] |= kSSLExtraFlag_DisallowSmallLights;

		if (ActiveLights == 0)
		{
			Source->unkFCPad[0] |= kSSLExtraFlag_NoActiveLights;
			Result = false;
		}

		return Result;
	}

	bool __stdcall ShadowRenderTasks::HandleLightProjectionStage2(ShadowSceneLight* Source)
	{
		// update extra flags and check if the main directional/sun/moon light is allowed
		SME_ASSERT(Source);

		if (CanHaveDirectionalShadow(Source) == false)
		{
			Source->unkFCPad[0] |= kSSLExtraFlag_DisallowDirectionalLight;
			return false;
		}
		else
			return true;
	}

	ShadowMapTexturePool			ShadowMapTexturePool::Instance;

	void ShadowMapTexturePool::Create( void )
	{
		SME_ASSERT(TexturePool[kPool_Tier1] == NULL);

		for (int i = kPool_Tier1; i < kPool__MAX; i++)
			TexturePool[i] = BSTextureManager::CreateInstance();
	}

	void ShadowMapTexturePool::Reset( void )
	{
		for (int i = kPool_Tier1; i < kPool__MAX; i++)
		{
			BSTextureManager* Instance = TexturePool[i];
			if (Instance)
				Instance->DeleteInstance();

			TexturePool[i] = NULL;
		}
	}

	void ShadowMapTexturePool::SetShadowMapResolution( UInt16 Resolution ) const
	{
		SME_ASSERT(Resolution <= 2048);

		UInt16* ResolutionPtr = (UInt16*)0x00B2C67C;
		*ResolutionPtr = Resolution;
	}

	void ShadowMapTexturePool::ReserveShadowMaps( BSTextureManager* Manager, UInt32 Count ) const
	{
		SME_ASSERT(Manager);

		Manager->ReserveShadowMaps(Count);
	}

	ShadowMapTexturePool::ShadowMapTexturePool()
	{
		TexturePool[kPool_Tier1] = NULL;
		TexturePool[kPool_Tier2] = NULL;
		TexturePool[kPool_Tier3] = NULL;

		PoolResolution[kPool_Tier1] = 1024;
		PoolResolution[kPool_Tier2] = 512;
		PoolResolution[kPool_Tier3] = 256;
	}

	ShadowMapTexturePool::~ShadowMapTexturePool()
	{
		Reset();
	}

	void ShadowMapTexturePool::Initialize( void )
	{
		UInt16 Tier1Res = Settings::kDynMapResolutionTier1().i;
		UInt16 Tier2Res = Settings::kDynMapResolutionTier2().i;
		UInt16 Tier3Res = Settings::kDynMapResolutionTier3().i;

		if (Tier3Res < 128)
			Tier3Res = 128;

		if (Tier1Res > 2048)
			Tier1Res = 2048;

		SME_ASSERT(Tier1Res && Tier2Res && Tier3Res);

		Create();
		PoolResolution[kPool_Tier1] = Tier1Res;
		PoolResolution[kPool_Tier2] = Tier2Res;
		PoolResolution[kPool_Tier3] = Tier3Res;

		_MESSAGE("Shadow Map Tiers => %d > %d > %d", Tier1Res, Tier2Res, Tier3Res);
	}

	void ShadowMapTexturePool::HandleShadowPass( NiDX9Renderer* Renderer, UInt32 MaxShadowCount ) const
	{
		for (int i = kPool_Tier1; i < kPool__MAX; i++)
		{
			SetShadowMapResolution(PoolResolution[i]);
			ReserveShadowMaps(TexturePool[i], MaxShadowCount);
		}
	}

	BSRenderedTexture* ShadowMapTexturePool::GetShadowMapTexture( ShadowSceneLight* Light ) const
	{
		SME_ASSERT(Light && Light->sourceNode);

		NiNode* Node = Light->sourceNode;
		TESObjectREFR* Object = Utilities::GetNodeObjectRef(Node);
		UInt16 DistancePool = kPool__MAX;
		UInt16 BoundPool = kPool__MAX;
		float Distance = Utilities::GetDistanceFromPlayer(Node);
		float Bound = Node->m_kWorldBound.radius;

		if (Settings::kDynMapEnableDistance().i)
		{
			if (Distance > 0 && Distance < Settings::kDynMapDistanceNear().f)
				DistancePool = kPool_Tier1;
			else if (Distance > Settings::kDynMapDistanceNear().f && Distance < Settings::kDynMapDistanceFar().f)
				DistancePool = kPool_Tier2;
			else
				DistancePool = kPool_Tier3;
		}

		if (Settings::kDynMapEnableBoundRadius().i)
		{
			if (Object && Object->IsActor())
				BoundPool = kPool_Tier1;			// actors get off easy
			else if (Bound < Settings::kObjectTier3BoundRadius().f)
				BoundPool = kPool_Tier3;
			else if (Bound < Settings::kObjectTier4BoundRadius().f)
				BoundPool = kPool_Tier2;
			else
				BoundPool = kPool_Tier1;
		}

		UInt16 PoolSelection = DistancePool;
		if (DistancePool == kPool__MAX)
			PoolSelection = BoundPool;

		if (DistancePool < kPool__MAX && BoundPool < kPool__MAX)
		{
			if (DistancePool > BoundPool)
				PoolSelection = DistancePool;
			else
				PoolSelection = BoundPool;
		}

		if (PoolSelection == kPool__MAX)
		{
			// we'll fallback to the middle ground if something went bollocks
			PoolSelection = kPool_Tier2;
		}

		if (Object == *g_thePlayer)
			PoolSelection = kPool_Tier1;

		SHADOW_DEBUG(Object, "Shadow Map Tier [D=%d,BR=%d] %d @ %dx", DistancePool + 1, BoundPool + 1, PoolSelection + 1, PoolResolution[PoolSelection]);

		BSTextureManager* Manager = TexturePool[PoolSelection];
		SME_ASSERT(Manager);

		BSRenderedTexture* Texture = Manager->FetchShadowMap();
		return Texture;
	}

	void ShadowMapTexturePool::DiscardShadowMapTexture( BSRenderedTexture* Texture ) const
	{
		bool Fallback = true;
		if (Texture)
		{
			if (Texture->renderTargets && Texture->renderTargets->targets[0])
			{
				UInt32 Width = Texture->renderTargets->targets[0]->width;
				BSTextureManager* Manager = GetPoolByResolution(Width);
				SME_ASSERT(Manager);

				Manager->DiscardShadowMap(Texture);
				Fallback = false;
			}
		}

		if (Fallback)
		{
			for (int i = kPool_Tier1; i < kPool__MAX; i++)
				TexturePool[i]->DiscardShadowMap(Texture);

			(*BSTextureManager::Singleton)->DiscardShadowMap(Texture);
		}
	}

	BSTextureManager* ShadowMapTexturePool::GetPoolByResolution( UInt16 Resolution ) const
	{
		for (int i = kPool_Tier1; i < kPool__MAX; i++)
		{
			if (PoolResolution[i] == Resolution)
				return TexturePool[i];
		}

		return NULL;
	}

	bool ShadowMapTexturePool::GetEnabled( void )
	{
		return Settings::kDynMapEnableDistance().i || Settings::kDynMapEnableBoundRadius().i;
	}

	_DefineHookHdlr(EnumerateFadeNodes, 0x00407508);
	_DefineHookHdlr(RenderShadowsProlog, 0x004073E4);
	_DefineHookHdlr(RenderShadowsEpilog, 0x00407AD3);
	_DefineHookHdlr(QueueModel3D, 0x00434BB2);
	_DefineHookHdlr(UpdateGeometryLighting, 0x00407945);
	_DefineHookHdlr(UpdateGeometryLightingSelf, 0x0040795C);
	_DefineHookHdlr(RenderShadowMap, 0x007D4E89);
	_DefineHookHdlr(CheckLargeObjectLightSource, 0x007D23F7);
	_DefineHookHdlr(CheckShadowReceiver, 0x007D692D);
	_DefineHookHdlr(CheckDirectionalLightSource, 0x007D282C);
	_DefineHookHdlr(ShadowSceneLightGetShadowMap, 0x007D4760);
	_DefineHookHdlr(CreateWorldSceneGraph, 0x0040EAAC);
	_DefinePatchHdlr(CullCellActorNodeA, 0x004076C1 + 1);
	_DefinePatchHdlr(CullCellActorNodeB, 0x004079D4);
	_DefineHookHdlr(BlacklistTreeNode, 0x0056118F);
	_DefinePatchHdlrWithBuffer(TrifleSupportPatch, 0x00407684, 5, 0xE8, 0x57, 0xF7, 0x3B, 0x0);
	_DefineHookHdlr(ShadowSceneLightCtor, 0x007D6122);
	_DefineHookHdlr(CalculateProjectionProlog, 0x007D22AD);
	_DefineHookHdlr(CalculateProjectionEpilog, 0x007D2DF2);
	_DefinePatchHdlr(ShadowLightShaderDepthBias, 0x007CB5D0 + 2);

	#define _hhName	EnumerateFadeNodes
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0040767D);
		__asm
		{
			mov		eax, [esp + 0x18]
			pushad
			push	eax
			call	ShadowRenderTasks::QueueShadowOccluders
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	RenderShadowsProlog
	_hhBegin()
	{
		_hhSetVar(Retn, 0x004073EC);
		__asm
		{
			mov     dword ptr [esp + 0x14], 0
			pushad
			call	ShadowRenderTasks::HandleMainProlog
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	RenderShadowsEpilog
	_hhBegin()
	{
		__asm
		{
			pushad
			call	ShadowRenderTasks::HandleMainEpilog
			popad

			pop		ebx
			add		esp, 0x4C
			retn
		}
	}

	#define _hhName	QueueModel3D
	_hhBegin()
	{
		_hhSetVar(Retn, 0x00434BB7);
		_hhSetVar(Call, 0x004A12E0);
		__asm
		{
			call	_hhGetVar(Call)

			pushad
			push	1
			push	eax
			call	ShadowRenderTasks::HandleModelLoad
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	UpdateGeometryLighting
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0040794A);
		__asm
		{
			pushad
			push	edx
			push	esi
			call	ShadowRenderTasks::HandleShadowLightUpdateReceiver
			popad

			pop		edx
			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	UpdateGeometryLightingSelf
	_hhBegin()
	{
		_hhSetVar(Retn, 0x00407961);
		_hhSetVar(Call, 0x007D6900);
		__asm
		{
			pushad
			push	ecx
			call	ShadowRenderTasks::HandleSelfShadowing
			test	al, al
			jz		SKIP

			popad
			call	_hhGetVar(Call)
			jmp		EXIT
		SKIP:
			popad
			pop		eax
		EXIT:
			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	RenderShadowMap
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D4E8E);
		_hhSetVar(Call, 0x0070C0B0);
		__asm
		{
			mov		eax, [esp + 0x18]
			pushad
			push	eax
			push	edi
			call	ShadowRenderTasks::HandleShadowMapRenderingProlog
			popad

			call	_hhGetVar(Call)

			mov		eax, [esp + 0x18]
			pushad
			push	eax
			push	edi
			call	ShadowRenderTasks::HandleShadowMapRenderingEpilog
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	CheckLargeObjectLightSource
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D2401);
		_hhSetVar(Jump, 0x007D272D);
		__asm
		{
			mov		[esp + 0x1C], ebx
			mov		eax, [esp + 0x48]

			pushad
			push	edi
			push	eax
			call	ShadowRenderTasks::HandleLightProjectionStage1
			test	al, al
			jz		SKIP

			popad
			jle		AWAY

			jmp		_hhGetVar(Retn)
	SKIP:
			popad
	AWAY:

			jmp		_hhGetVar(Jump)
		}
	}

	#define _hhName	CheckShadowReceiver
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D6935);
		__asm
		{
			pushad
			push	ecx
			push	edi
			call	ShadowRenderTasks::HandleShadowReceiverLightingPropertyUpdate
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	CheckDirectionalLightSource
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D2835);
		_hhSetVar(Jump, 0x007D2872);
		__asm
		{
			jp		AWAY
			mov     ecx, [esp + 0xBC]

			mov		eax, [esp + 0x48]
			pushad
			push	eax
			call	ShadowRenderTasks::HandleLightProjectionStage2
			test	al, al
			jz		SKIP

			popad
			jmp		_hhGetVar(Retn)
	SKIP:
			popad
	AWAY:
			jmp		_hhGetVar(Jump)
		}
	}

	#define _hhName	TextureManagerDiscardShadowMap
	_hhBegin()
	{
		__asm
		{
			lea		ecx, ShadowMapTexturePool::Instance
			jmp		ShadowMapTexturePool::DiscardShadowMapTexture
		}
	}

	#define _hhName	TextureManagerReserveShadowMaps
	_hhBegin()
	{
		__asm
		{
			lea		ecx, ShadowMapTexturePool::Instance
			jmp		ShadowMapTexturePool::HandleShadowPass
		}
	}

	#define _hhName	ShadowSceneLightGetShadowMap
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D4765);
		__asm
		{
			lea		ecx, ShadowMapTexturePool::Instance
			push	ebx
			call	ShadowMapTexturePool::GetShadowMapTexture

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	CreateWorldSceneGraph
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0040EAB1);
		_hhSetVar(Call, 0x00406950);
		__asm
		{
			call	_hhGetVar(Call)

			lea		ecx, ShadowMapTexturePool::Instance
			call	ShadowMapTexturePool::Initialize

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	BlacklistTreeNode
	_hhBegin()
	{
		_hhSetVar(Retn, 0x00561194);
		_hhSetVar(Call, 0x005640E0);
		__asm
		{
			call	_hhGetVar(Call)

			pushad
			push	eax
			call	ShadowRenderTasks::HandleTreeModelLoad
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	ShadowSceneLightCtor
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D6127);
		_hhSetVar(Call, 0x00716DB0);
		__asm
		{
			call	_hhGetVar(Call)

			pushad
			push	esi
			call	ShadowRenderTasks::HandleSSLCreation
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	CalculateProjectionProlog
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D22B3);
		__asm
		{
			mov		esi, ecx
			mov		[esp + 0x48], esi

			pushad
			push	esi
			call	ShadowRenderTasks::HandleLightProjectionProlog
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	CalculateProjectionEpilog
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D2DF8);
		__asm
		{
			mov		eax, [esi + 0x100]

			pushad
			push	esi
			call	ShadowRenderTasks::HandleLightProjectionEpilog
			popad

			jmp		_hhGetVar(Retn)
		}
	}

#ifndef NDEBUG
	void __stdcall TraceSLLCulling(ShadowSceneLight* Source, int Stage, int Result)
	{
		if (Source->sourceNode == NULL)
			return;

		TESObjectREFR* Ref = Utilities::GetNodeObjectRef(Source->sourceNode);
		if (Ref == NULL)
			return;

		switch (Stage)
		{
		case 1:
			SHADOW_DEBUG(Ref, "0x007D34C0 returned %d | Fade[%f,%f] Bnd[%f,%f]",
						Result,
						Source->unkDC,
						Source->unkE0,
						Source->m_combinedBounds.z,
						Source->m_combinedBounds.radius);

			break;
		case 2:
			SHADOW_DEBUG(Ref, "0x0047DA70 returned %d | Fade[%f,%f] Bnd[%f,%f]",
						Result,
						Source->unkDC,
						Source->unkE0,
						Source->m_combinedBounds.z,
						Source->m_combinedBounds.radius);

			break;
		case 3:
			SHADOW_DEBUG(Ref, "0x007415E0 returned %d | Fade[%f,%f] Bnd[%f,%f]",
						Result,
						Source->unkDC,
						Source->unkE0,
						Source->m_combinedBounds.z,
						Source->m_combinedBounds.radius);

			break;
		case 4:
			SHADOW_DEBUG(Ref, "0x007D5B20 returned %d | Fade[%f,%f] Bnd[%f,%f]",
						Result,
						Source->unkDC,
						Source->unkE0,
						Source->m_combinedBounds.z,
						Source->m_combinedBounds.radius);

			break;
		}
	}

	#define _hhName	SSLTraceCullingA
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D6410);
		_hhSetVar(Call, 0x007D34C0);
		__asm
		{
			call	_hhGetVar(Call)

			pushad
			movzx	eax, al
			push	eax
			push	1
			push	ebp
			call	TraceSLLCulling
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	SSLTraceCullingB
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D647F);
		_hhSetVar(Call, 0x0047DA70);
		__asm
		{
			call	_hhGetVar(Call)

			pushad
			push	eax
			push	2
			push	ebp
			call	TraceSLLCulling
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	SSLTraceCullingC
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D6496);
		_hhSetVar(Call, 0x007415E0);
		__asm
		{
			call	_hhGetVar(Call)

			pushad
			movzx	eax, al
			push	eax
			push	3
			push	ebp
			call	TraceSLLCulling
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	#define _hhName	SSLTraceCullingD
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007D651C);
		_hhSetVar(Call, 0x007D5B20);
		__asm
		{
			call	_hhGetVar(Call)

			pushad
			push	eax
			push	4
			push	ebp
			call	TraceSLLCulling
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	_DefineHookHdlr(SSLTraceCullingA, 0x007D640B);
	_DefineHookHdlr(SSLTraceCullingB, 0x007D647A);
	_DefineHookHdlr(SSLTraceCullingC, 0x007D6491);
	_DefineHookHdlr(SSLTraceCullingD, 0x007D6517);
#endif

#if 0
	_DeclareMemHdlr(TestHook4, "");
	_DefineHookHdlr(TestHook4, 0x007A9845);

	bool __stdcall CheckRenderPass(void)
	{
		static NiGeometry* Buffer = NULL;
		static int CasterCounter = 0, LastCounter = 0;
		BSRenderPassData* Current = *(BSRenderPassData**)0x00B42EB8;
		bool Render = true;

		NiNode* Parent = (NiNode*)Current->geom->m_parent;
		if (Current->type >= 375 && Current->type <= 378)
		{
			if (Buffer != Current->geom)
			{
	//			if (CasterCounter != LastCounter)
	//				_MESSAGE("Mismatch in caster counter! Expected = %d, Found = %d", CasterCounter, LastCounter);

				Buffer = Current->geom;
	//			ShadowLightListT Lights;
	//			CasterCounter = Utilities::GetNodeActiveLights(Parent, &Lights, Utilities::ActiveShadowSceneLightEnumerator::kParam_ShadowCasters);
				LastCounter = 0;

	//			_MESSAGE("PASS = %s | GEOM = %s | PARENT = %s | Casters = %d",
	//					 Current->GetRenderPassName(), Current->geom->m_pcName, Parent->m_pcName, CasterCounter);
			}
			else if (ShadowSundries::kDebugSelection)
				Render = false;

	//		gLog.Indent();
			if (Current->unk08)
			{
				LastCounter += Current->unk08;
	//			if (Current->unk08 > 1)
	//				_MESSAGE("SHADOW PASS: %d casters...", Current->unk08);

				for (int i = 0; i < Current->unk08; i++)
				{
					ShadowSceneLight* Light = Current->unk0C[i];
	//				_MESSAGE("Caster = %s %s", Light->sourceNode->m_pcName, (Render == false ? "(SKIP)" : ""));
				}
			}

	//		gLog.Outdent();
		}

		return Render;
	}

	#define _hhName	TestHook4
	_hhBegin()
	{
		_hhSetVar(Retn, 0x007A984A);
		_hhSetVar(Skip, 0x007A98C3);
		_hhSetVar(Call, 0x00707530);
		__asm
		{
			call	_hhGetVar(Call)
			pushad
			call	CheckRenderPass
			test	al, al
			jz		SKIP
			popad
			jmp		_hhGetVar(Retn)
		SKIP:
			popad
			jmp		_hhGetVar(Skip)
		}
	}

	void __stdcall TagMainPass(bool start)
	{
		if (start)
		{
			_MESSAGE("BEGIN MAIN PASS===========================================================================");
			gLog.Indent();
		}
		else
		{
			_MESSAGE("==========================================================================END MAIN PASS");
			gLog.Outdent();
		}
	}

	_DeclareMemHdlr(TestHook5, "");
	_DefineHookHdlr(TestHook5, 0x0040CCD3);
	#define _hhName	TestHook5
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0040CCD8);
		_hhSetVar(Call, 0x0070C0B0);
		__asm
		{
			pushad
			push 1
			call	TagMainPass
			popad

			call	_hhGetVar(Call)

			pushad
			push 0
			call	TagMainPass
			popad

			jmp		_hhGetVar(Retn)
		}
	}

	_DeclareMemHdlr(TestHook6, "");
	_DefineHookHdlr(TestHook6, 0x0040CE48);
	#define _hhName	TestHook6
	_hhBegin()
	{
		_hhSetVar(Retn, 0x0040CE4D);
		_hhSetVar(Call, 0x0070C0B0);
		__asm
		{
			pushad
			push 1
			call	TagMainPass
			popad
			// render player FP node
			call	_hhGetVar(Call)

			pushad
			push 0
			call	TagMainPass
			popad

			jmp		_hhGetVar(Retn)
		}
	}
#endif
	void Patch(void)
	{
#ifndef NDEBUG
		_MemHdlr(SSLTraceCullingA).WriteJump();
		_MemHdlr(SSLTraceCullingB).WriteJump();
		_MemHdlr(SSLTraceCullingC).WriteJump();
		_MemHdlr(SSLTraceCullingD).WriteJump();
#endif

#if 0
		_MemHdlr(TestHook4).WriteJump();
		_MemHdlr(TestHook5).WriteJump();
	//	_MemHdlr(TestHook6).WriteJump();
#endif
		_MemHdlr(EnumerateFadeNodes).WriteJump();
		_MemHdlr(RenderShadowsProlog).WriteJump();
		_MemHdlr(RenderShadowsEpilog).WriteJump();
		_MemHdlr(QueueModel3D).WriteJump();
		_MemHdlr(UpdateGeometryLighting).WriteJump();
		_MemHdlr(UpdateGeometryLightingSelf).WriteJump();
		_MemHdlr(RenderShadowMap).WriteJump();
		_MemHdlr(CheckLargeObjectLightSource).WriteJump();
		_MemHdlr(CheckShadowReceiver).WriteJump();
		_MemHdlr(CheckDirectionalLightSource).WriteJump();
		_MemHdlr(BlacklistTreeNode).WriteJump();
		_MemHdlr(ShadowSceneLightCtor).WriteJump();
		_MemHdlr(CalculateProjectionProlog).WriteJump();
		_MemHdlr(CalculateProjectionEpilog).WriteJump();
		_MemHdlr(ShadowLightShaderDepthBias).WriteUInt32((UInt32)&ShadowRenderTasks::ShadowDepthBias);

		if (Settings::kActorsReceiveAllShadows().i)
		{
			_MemHdlr(CullCellActorNodeA).WriteUInt8(1);
			_MemHdlr(CullCellActorNodeB).WriteUInt8(0xEB);
		}

		if (ShadowMapTexturePool::GetEnabled())
		{
			for (int i = 0; i < 5; i++)
			{
				static const UInt32 kCallSites[5] =
				{
					0x007C5C6E,	0x007C5F19,
					0x007D52CD, 0x007D688B,
					0x007D5557
				};

				_DefineHookHdlr(TextureManagerDiscardShadowMap, kCallSites[i]);
				_MemHdlr(TextureManagerDiscardShadowMap).WriteCall();
			}

			for (int i = 0; i < 2; i++)
			{
				static const UInt32 kCallSites[2] =
				{
					0x0040746D,	0x004074F3
				};

				_DefineHookHdlr(TextureManagerReserveShadowMaps, kCallSites[i]);
				_MemHdlr(TextureManagerReserveShadowMaps).WriteCall();
			}

			_MemHdlr(ShadowSceneLightGetShadowMap).WriteJump();
			_MemHdlr(CreateWorldSceneGraph).WriteJump();
		}
	}

	void Initialize( void )
	{
		MainShadowExParams::Instance.Initialize();
		SelfShadowExParams::Instance.Initialize();
		ShadowReceiverExParams::Instance.Initialize();
		ShadowRenderTasks::Initialize();
	}
}