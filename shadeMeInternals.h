#pragma once

#include "obse/PluginAPI.h"
#include "obse/CommandTable.h"
#include "obse/GameAPI.h"
#include "obse/GameObjects.h"
#include "obse/GameData.h"
#include "obse/GameMenus.h"
#include "obse/GameOSDepend.h"
#include "obse/GameExtraData.h"
#include "obse/NiAPI.h"
#include "obse/NiObjects.h"
#include "obse/NiTypes.h"
#include "obse/ParamInfos.h"
#include "obse/GameActorValues.h"
#include "obse/GameProcess.h"
#include "obse/NiControllers.h"
#include "obse/NiNodes.h"
#include "obse/NiObjects.h"
#include "obse/NiExtraData.h"
#include "obse/NiRTTI.h"
#include "obse/NiRenderer.h"

#include <SME_Prefix.h>
#include <MemoryHandler.h>
#include <INIManager.h>
#include <StringHelpers.h>
#include <MersenneTwister.h>
#include <MiscGunk.h>

#include <d3d9.h>

using namespace SME;
using namespace SME::MemoryHandler;

namespace Interfaces
{
	extern PluginHandle						kOBSEPluginHandle;

	extern const OBSEInterface*				kOBSE;
	extern OBSEMessagingInterface*			kOBSEMessaging;
}

class shadeMeINIManager : public INI::INIManager
{
public:
	virtual ~shadeMeINIManager();

	virtual void							Initialize(const char* INIPath, void* Parameter);

	static shadeMeINIManager				Instance;
};

namespace Settings
{
	extern SME::INI::INISetting				kCasterMaxDistance;
	extern SME::INI::INISetting				kEnableDebugShader;
	extern SME::INI::INISetting				kEnableDetailedDebugSelection;
	extern SME::INI::INISetting				kForceActorShadows;
	extern SME::INI::INISetting				kNoInteriorSunShadows;
	extern SME::INI::INISetting				kActorsReceiveAllShadows;
	extern SME::INI::INISetting				kNightTimeMoonShadows;
	extern SME::INI::INISetting				kBoundRadiusWeight;
	extern SME::INI::INISetting				kReduceGridSearch;
	extern SME::INI::INISetting				kExteriorDistanceCheck;

	extern SME::INI::INISetting				kLargeObjectHigherPriority;
	extern SME::INI::INISetting				kLargeObjectExcludedPath;
	extern SME::INI::INISetting				kLargeObjectSunShadowsOnly;
	extern SME::INI::INISetting				kLargeObjectDistanceWeight;

	extern SME::INI::INISetting				kRenderBackfacesIncludePath;

	extern SME::INI::INISetting				kMainExcludedTypesInterior;
	extern SME::INI::INISetting				kMainExcludedTypesExterior;

	extern SME::INI::INISetting				kMainExcludedPathInterior;
	extern SME::INI::INISetting				kMainExcludedPathExterior;

	extern SME::INI::INISetting				kLightLOSCheckInterior;
	extern SME::INI::INISetting				kLightLOSCheckExterior;
	extern SME::INI::INISetting				kLightLOSSkipLargeObjects;
	extern SME::INI::INISetting				kLightLOSExcludedPath;
	extern SME::INI::INISetting				kLightLOSSkipActors;

	extern SME::INI::INISetting				kPlayerLOSCheckInterior;
	extern SME::INI::INISetting				kPlayerLOSCheckExterior;
	extern SME::INI::INISetting				kPlayerLOSCheckHighAccuracy;
	extern SME::INI::INISetting				kPlayerLOSCheckThresholdDist;

	extern SME::INI::INISetting				kSelfExcludedTypesInterior;
	extern SME::INI::INISetting				kSelfExcludedTypesExterior;

	extern SME::INI::INISetting				kSelfExcludedPathInterior;
	extern SME::INI::INISetting				kSelfExcludedPathExterior;

	extern SME::INI::INISetting				kReceiverExcludedTypesInterior;
	extern SME::INI::INISetting				kReceiverExcludedTypesExterior;

	extern SME::INI::INISetting				kReceiverExcludedPathInterior;
	extern SME::INI::INISetting				kReceiverExcludedPathExterior;

	extern SME::INI::INISetting				kReceiverEnableExclusionParams;

	extern SME::INI::INISetting				kInteriorHeuristicsEnabled;
	extern SME::INI::INISetting				kInteriorHeuristicsIncludePath;
	extern SME::INI::INISetting				kInteriorHeuristicsExcludePath;

	extern SME::INI::INISetting				kObjectTier1BoundRadius;		// anything smaller won't cast shadows
	extern SME::INI::INISetting				kObjectTier2BoundRadius;		// interior light LOS checks are limited to tier 2 or lower
	extern SME::INI::INISetting				kObjectTier3BoundRadius;		// casters b'ween tier 2 and 3 use different projection params
	extern SME::INI::INISetting				kObjectTier4BoundRadius;		// minimum radius need to qualify for interior heuristics
	extern SME::INI::INISetting				kObjectTier5BoundRadius;		// exterior player LOS checks are limited to tier 5 or lower
	extern SME::INI::INISetting				kObjectTier6BoundRadius;		// large objects

	extern SME::INI::INISetting				kSelfIncludePathInterior;
	extern SME::INI::INISetting				kSelfIncludePathExterior;

	extern SME::INI::INISetting				kDynMapEnableDistance;
	extern SME::INI::INISetting				kDynMapEnableBoundRadius;

	extern SME::INI::INISetting				kDynMapResolutionTier1;
	extern SME::INI::INISetting				kDynMapResolutionTier2;
	extern SME::INI::INISetting				kDynMapResolutionTier3;

	extern SME::INI::INISetting				kDynMapDistanceNear;
	extern SME::INI::INISetting				kDynMapDistanceFar;

	extern SME::INI::INISetting				kWeatherDisableCloudy;
	extern SME::INI::INISetting				kWeatherDisableRainy;
	extern SME::INI::INISetting				kWeatherDisableSnow;

	extern SME::INI::INISetting				kWeatherDiffuseCloudy;
	extern SME::INI::INISetting				kWeatherDiffuseRainy;
	extern SME::INI::INISetting				kWeatherDiffuseSnow;

	extern SME::INI::INISetting				kMaxCountActor;
	extern SME::INI::INISetting				kMaxCountBook;
	extern SME::INI::INISetting				kMaxCountFlora;
	extern SME::INI::INISetting				kMaxCountIngredient;		// includes sigil stones, soul gems
	extern SME::INI::INISetting				kMaxCountMiscItem;			// includes keys
	extern SME::INI::INISetting				kMaxCountAlchemyItem;
	extern SME::INI::INISetting				kMaxCountEquipment;			// includes armor, weapon, clothing, ammo
	extern SME::INI::INISetting				kMaxCountLargeObject;		

	extern SME::INI::INISetting				kMiscForceSM3RenderPath;
}

class BSRenderedTexture;
class NiRenderedTexture;
class NiDX9Renderer;

// 60
class NiDX9TextureData
{
public:
	NiDX9TextureData();
	~NiDX9TextureData();

	// 44
	// ### are all members signed?
	struct Unk0C
	{								//			initialized to
		UInt8			unk00;		// 00		1
		UInt8			pad00[3];
		UInt32			unk04;		// 04		2
		UInt32			unk08;		// 08		0
		SInt32			unk0C;		// 0C		-1
		UInt32			unk10;		// 10		0
		UInt32			unk14;		// 14		16
		UInt32			unk18;		// 18		3
		UInt8			unk1C;		// 1C		8
		UInt8			pad1C[3];
		UInt32			unk20;		// 20		19
		UInt32			unk24;		// 24		5
		UInt8			unk28;		// 28		0
		UInt8			unk29;		// 29		1
		UInt8			pad2A[2];
		UInt32			unk2C;		// 2C		19
		UInt32			unk30;		// 30		5
		UInt8			unk34;		// 34		0
		UInt8			unk35;		// 35		1
		UInt8			pad36[2];
		UInt32			unk38;		// 38		19
		UInt32			unk3C;		// 3C		5
		UInt8			unk40;		// 40		0
		UInt8			unk41;		// 41		1
		UInt8			pad42[2];
	};

	//void*							vtbl;			// 00
	NiRenderedTexture*				unk04;			// 04	parent texture
	NiDX9Renderer*					unk08;			// 08	parent renderer
	Unk0C							unk0C;			// 0C
	UInt32							unk50;			// 50
	UInt32							unk54;			// 54
	UInt32							surfaceWidth;	// 58
	UInt32							surfaceHeight;	// 5C
};

// 64
class NiDX9RenderedTextureData : public NiDX9TextureData
{
public:
	NiDX9RenderedTextureData();
	~NiDX9RenderedTextureData();

	UInt32							unk60;			// 60
};

// BSShaderProperty::RenderPass
// 10
class BSRenderPassData
{
public:
	NiGeometry*				geom;		// 00
	UInt16					type;		// 04
	UInt8					isFPass;	// 06
	UInt8					unk07;		// 07 - init to 0
	UInt8					unk08;		// 08 - size of unk0C array
	UInt8					unk09;		// 09
	UInt16					pad0A;
	ShadowSceneLight**		unk0C;		// 0C

	const char*				GetRenderPassName();
};

// 64
struct BVIntersectionData
{
	NiPlane							planes[6];				// 00
	UInt32							intersections;			// 60 - bit field, mask = 0x3F (6 bits)
};
STATIC_ASSERT(sizeof(BVIntersectionData) == 0x64);

// 220
class ShadowSceneLight : public NiNode
{
public:
	ShadowSceneLight();
	~ShadowSceneLight();

	float												unkDC;		// time left before full fade-in opacity?
	float												unkE0;		// time elapsed during fade-in?
	NiTPointerList<NiTriBasedGeom>						unkE4;		// light receiver geometry?
	UInt8												unkF4;		// shadow map rendered/casts shadow flag?
	UInt8												unkF5;		// use cubemap camera when set?
	UInt8												unkF5Pad[2];
	float												unkF8;
	UInt8												unkFC;
	UInt8												unkFCPad[3];
	NiPointLight*										sourceLight;// 100 - parent light
	UInt8												unk104;
	UInt8												unk104Pad[3];
	NiVector3											unk108;		// sourceLight->m_worldTranslate
	BSRenderedTexture*									shadowMap;	// 114 - shadow map texture
	UInt16												unk118;		// when 0xFF, light source is culled (not active)
	UInt16												unk11A;
	UInt32												unk11C;
	UInt8												unk120;		// force render shadow map?
	UInt8												pad121[3];
	NiPointer<BSCubeMapCamera>							unk124;		// light camera?
	UInt32												unk128;
	UInt8												showDebug;	// 12C - debug shader toggle
	UInt8												unk12CPad[3];
	NiNode*												sourceNode;	// 130 - node being lighted/shadowed
	NiTPointerList<NiAVObject>							unk134;
	NiTPointerList<NiTriBasedGeom>::Node*				unk144;		// points to the fence trishape
	NiPointer<NiTriShape>								unk148;		// name set as "fence"
	NiCamera*											unk14C;		// used when performing LOS checks/frustum culling
	BVIntersectionData									unk150;		// used when updating shadow receiver geometry
	BVIntersectionData									unk1B4;
	UInt32												unk218;
	UInt32												unk21C;
};
STATIC_ASSERT(offsetof(ShadowSceneLight, sourceLight) == 0x100);
STATIC_ASSERT(offsetof(ShadowSceneLight, sourceNode) == 0x130);
STATIC_ASSERT(offsetof(ShadowSceneLight, unk150) == 0x150);
STATIC_ASSERT(sizeof(ShadowSceneLight) == 0x220);

// 24
class BSRenderedTexture : public NiRefObject
{
public:
	// members
	///*00*/ NiRefObject
	/*08*/ NiRenderTargetGroup*		renderTargets;
	/*0C*/ UInt32					unk0C;
	/*10*/ UInt32					unk10;
	/*14*/ UInt32					unk14;
	/*18*/ UInt32					unk18;
	/*1C*/ UInt32					unk1C;
	/*20*/ NiRenderedTexture*		renderedTexture;
};

// manages off-screen render targets
// 48
class BSTextureManager
{
public:
	// ?
	struct RenderedTextureData
	{
		UInt32		unk00;
	};

	NiTPointerList<RenderedTextureData>				unk00;				// 00
	NiTPointerList<RenderedTextureData>				unk10;				// 10
	NiTPointerList<BSRenderedTexture>				shadowMaps;			// 20
	NiTPointerList<BSRenderedTexture>				unk30;				// 30
	void*											unk40;				// 40 - smart pointer, screenshot rendertarget?
	void*											unk44;				// 44 - smart pointer

	// methods
	BSRenderedTexture*								FetchShadowMap(void);
	void											DiscardShadowMap(BSRenderedTexture* Texture);
	void											ReserveShadowMaps(UInt32 Count);

	BSRenderedTexture*								GetDefaultRenderTarget(UInt32 Type);

	static BSTextureManager*						CreateInstance(void);
	void											DeleteInstance(void);

	static BSTextureManager**						Singleton;
};
STATIC_ASSERT(sizeof(BSTextureManager) == 0x48);

typedef std::vector<ShadowSceneLight*>			ShadowLightListT;
typedef std::vector<BSFadeNode*>				FadeNodeListT;
typedef std::vector<NiNode*>					NiNodeListT;

// ?
class BSTreeNode : public NiNode
{
public:
};

namespace Utilities
{
	float				GetDistanceFromPlayer(NiAVObject* Source);
	bool				GetPlayerHasLOS(TESObjectREFR* Target, bool HighAccuracy = false);	// slooooooooowwwwww!
	bool				GetLightLOS(NiAVObject* Light, TESObjectREFR* Target);				// slooooooooooooooooooooowwwwwwwwwwwwwwwwwwwwwwwwweeeerrrrrrrr!
																							// also, haaaaaaaaaaaaccccckkkkkyyyyy!
	bool				GetAbovePlayer(TESObjectREFR* Ref, float Threshold);
	bool				GetBelowPlayer(TESObjectREFR* Ref, float Threshold);
	bool				GetConsoleOpen(void);
	ShadowSceneNode*	GetShadowSceneNode(void);

	NiObjectNET*		GetNiObjectByName(NiObjectNET* Source, const char* Name);
	NiExtraData*		GetNiExtraDataByName(NiAVObject* Source, const char* Name);
	NiProperty*			GetNiPropertyByID(NiAVObject* Source, UInt8 ID);
	UInt32				GetNodeActiveLights(NiNode* Source, ShadowLightListT* OutList, UInt32 Params);
	BSFadeNode*			GetPlayerNode(bool FirstPerson = false);
	UInt8				GetWeatherClassification(TESWeather* Weather);

	void				UpdateBounds(NiNode* Node);
	float				GetDistance(NiAVObject* Source, NiAVObject* Destination);
	float				GetDistance(Vector3* Source, Vector3* Destination);
	ShadowSceneLight*	GetShadowCasterLight(NiNode* Caster);
	BSXFlags*			GetBSXFlags(NiAVObject* Source, bool Allocate = false);
	TESObjectREFR*		GetNodeObjectRef(NiAVObject* Source);

	void*				NiRTTI_Cast(const NiRTTI* TypeDescriptor, NiRefObject* NiObject);

	template <typename T>
	class DelimitedINIStringList
	{
	public:
		typedef std::vector<T>		ParameterListT;
	protected:

		ParameterListT				Params;
		std::string					Delimiter;

		void Clear(void)
		{
			Params.clear();
		}

		void Parse(const INI::INISetting* Setting)
		{
			SME::StringHelpers::Tokenizer Parser((*Setting)().s, Delimiter.c_str());
			std::string CurrentArg = "";

			while (Parser.NextToken(CurrentArg) != -1)
			{
				if (CurrentArg.length())
					HandleParam(CurrentArg.c_str());
			}
		}

		virtual void HandleParam(const char* Param) = 0;		// called for each parsed token
	public:
		DelimitedINIStringList(const char* Delimiters) :
			Params(),
			Delimiter(Delimiters)
		{
			SME_ASSERT(Delimiter.length());
		}

		virtual ~DelimitedINIStringList()
		{
			Clear();
		}

		void Refresh(const INI::INISetting* Source)				// loads params into the data store
		{
			SME_ASSERT(Source && Source->GetType() == INI::INISetting::kType_String);

			Clear();
			Parse(Source);
		}

		const ParameterListT& operator()(void) const
		{
			return Params;
		}

		virtual void Dump(void) const = 0;
	};

	class IntegerINIParamList : public DelimitedINIStringList<int>
	{
	protected:
		virtual void			HandleParam(const char* Param);
	public:
		IntegerINIParamList(const char* Delimiters = " ,");
		virtual ~IntegerINIParamList();

		virtual void			Dump(void) const;
	};

	class FilePathINIParamList : public DelimitedINIStringList<std::string>
	{
	protected:
		virtual void			HandleParam(const char* Param);
	public:
		FilePathINIParamList(const char* Delimiters = ",");
		virtual ~FilePathINIParamList();

		virtual void			Dump(void) const;
	};

	class NiNodeChildVisitor
	{
	public:
		virtual ~NiNodeChildVisitor() = 0
		{
			;//
		}

		virtual bool			AcceptBranch(NiNode* Node) = 0;					// for each child NiNode, return false to skip traversal
		virtual void			AcceptLeaf(NiAVObject* Object) = 0;				// for each child NiAVObject that isn't a NiNode
	};

	class NiNodeChildrenWalker
	{
		NiNode*					Root;
		NiNodeChildVisitor*		Visitor;

		void					Traverse(NiNode* Branch);
	public:
		NiNodeChildrenWalker(NiNode* Source);
		~NiNodeChildrenWalker();

		void					Walk(NiNodeChildVisitor* Visitor);
	};

	class ActiveShadowSceneLightEnumerator : public Utilities::NiNodeChildVisitor
	{
	protected:
		ShadowLightListT*		ActiveLights;
		UInt32					Param;
	public:
		enum
		{
			kParam_NonShadowCasters		= 0,
			kParam_ShadowCasters		= 1,
			kParam_Both					= 2,
		};

		ActiveShadowSceneLightEnumerator(ShadowLightListT* OutList, UInt32 Params);
		virtual ~ActiveShadowSceneLightEnumerator();

		virtual bool			AcceptBranch(NiNode* Node);
		virtual void			AcceptLeaf(NiAVObject* Object);
	};
}

#define NI_CAST(obj, to)		(to##*)Utilities::NiRTTI_Cast(NiRTTI_##to, obj)
