#pragma once

typedef struct
{
	int clientnum;
	int localClientnum;
	// ...
} cg_t;

typedef struct
{
	char			unknown0[2]; // 0
	BYTE			currentValid; // 2
	char			unknown3[0xD]; // 3
	int				killcamRagdollHandle; // 0x10
	int				ragdollHandle; // 0x14
	int				unknown32; // 0x18
	vec3_t			lerpOrigin; // 0x1C
	vec3_t			lerpAngles; // 0x28
	char			unknown52[0xC]; // 0x34
	int* unknown12; // 0x40
	char			unknown321[0x30]; // 0x44
	vec3_t			newOrigin; // 0x74
	char			unknown128[76]; //0x80
	int				clientNum; // 0xCC
	DWORD			eType; // 0xD0
	DWORD			eFlags; // 0xD4
	char			unknown216[12]; // 0xD8
	vec3_t			oldOrigin; // 0xE4
	char			unknown240[0x68]; // 0xF0
	int				parentEntity; // 0x158
	char			unknown532[0x34]; // 0x15C
	int				weapon; // 0x190
	int				weaponModel; // 0x194
	char			unknown404[40]; // 0x198
	int				isAlive; // 0x1C0
	char			unknown452[24];
} centity_t; // 0x1DC

struct XSurfaceCollisionAabb {

	unsigned short mins[3];
	unsigned short maxs[3];
};

struct XSurfaceCollisionNode {

	XSurfaceCollisionAabb aabb;
	unsigned short childBeginIndex;
	unsigned short childCount;
};

struct XSurfaceCollisionLeaf {

	unsigned short triangleBeginIndex;
};

struct XSurfaceCollisionTree {

	float trans[3];
	float scale[3];
	unsigned int nodeCount;
	XSurfaceCollisionNode* nodes;
	unsigned int leafCount;
	XSurfaceCollisionLeaf* leafs;
};

struct XRigidVertList {

	unsigned short boneOffset;
	unsigned short vertCount;
	unsigned short triOffset;
	unsigned short triCount;
	XSurfaceCollisionTree* collisionTree;
};

struct XSurfaceVertexInfo
{

	short vertCount[4];
	unsigned short* vertsBlend;
};

union PackedUnitVec {

	unsigned int packed;
};

struct GfxPackedVertex
{

	float xyz[3];
	float binormalSign;
	char color[4];
	PackedUnitVec normal;
	float uv[2];
};

struct XSurface
{
	char tileMode; // 0x0
	char deformed; // 0x1 (bool)
	unsigned short vertCount; // 0x2
	unsigned short triCount; // 0x4
	char pad[6]; // 0x6
	unsigned short* triIndices;	// 0xC
	XSurfaceVertexInfo vertInfo;
	GfxPackedVertex* verts;
	unsigned int vertListCount;	// 0x20
	XRigidVertList* vertList;	// 0x24
	char pad2[0x10]; // 0x28
};

struct  MaterialInfo
{
	const char* name; // 0x0
	char gameFlags; // 0x4
	char sortKey; // 0x5
	char textureAtlasRowCount; // 0x6
	char textureAtlasColumnCount; // 0x7
	char drawSurf[8]; // 0x8
	unsigned int surfaceTypeBits; // 0x10
}; // size 0x14

struct MaterialPass
{
	void* vertexDecl;
	void* vertexShaderArray[15];
	void* vertexShader;
	void* pixelShader;
	char perPrimArgCount;
	char perObjArgCount;
	char stableArgCount;
	char customSamplerFlags;
	char precompiledIndex;
	void* args;
};

/* 6902 */
struct MaterialTechnique
{
	const char* name;
	unsigned __int16 flags;
	unsigned __int16 passCount;
	MaterialPass passArray[1];
};

struct MaterialTechniqueSet
{
	const char* name;
	char worldVertFormat;
	char unused[2];
	MaterialTechniqueSet* remappedTechniqueSet;
	MaterialTechnique* techniques[26];
};

enum MapType
{
	MAPTYPE_NONE = 0x0,
	MAPTYPE_INVALID1 = 0x1,
	MAPTYPE_INVALID2 = 0x2,
	MAPTYPE_2D = 0x3,
	MAPTYPE_3D = 0x4,
	MAPTYPE_CUBE = 0x5,
	MAPTYPE_COUNT = 0x6,
};

struct GfxImage
{
	MapType mapType; // 0x0
	char junk[0x1C]; // 0x4
	const char* name; // 0x20
}; // size unreliable

struct water_t
{
	char padding[0x20];
	GfxImage* image;
};

/* 6909 */
struct MaterialWaterDef
{
	char padding[0x40]; // 0x0
	water_t* map; // 0x40
};


/* 6910 */
union MaterialTextureDefInfo
{
	GfxImage* image;
	MaterialWaterDef* water;
};

/* 6911 */
struct MaterialTextureDef
{
	unsigned int nameHash;
	char nameStart;
	char nameEnd;
	char samplerState;
	char semantic;
	MaterialTextureDefInfo u;
};

struct MaterialConstantDef
{
	unsigned int nameHash;
	char name[12];
	float literal[4];
};

struct GfxStateBits
{
	unsigned int loadBits[2];
};

struct Material
{
	MaterialInfo info; // 0x0
	char stateBitsEntry[26]; // 0x14
	char junk[12]; // 0x2E
	char textureCount; // 0x3A
	char junkv2[5]; // 0x3B
	MaterialTechniqueSet* techniqueSet; // 0x40
	MaterialTextureDef* textureTable; // 0x44
	MaterialConstantDef* constantTable;
	GfxStateBits* stateBitsTable;
};

struct XModelLodInfo
{
	float dist;
	unsigned short numsurfs;
	unsigned short surfIndex;
	int partBits[4];
};

struct XBoneInfo
{
	float bounds[2][3];
	float offset[3];
	float radiusSquared;
};

struct XModelHighMipBounds {

	float mins[3];
	float maxs[3];
};

struct XModelStreamInfo {

	XModelHighMipBounds* highMipBounds;
};

struct XModel
{
	const char* name; // 0x0
	char numBones; // 0x4
	char numRootBones; // 0x5
	char numSurfaces; // 0x6
	char pad; // 0x7
	uint16_t* boneNames; // 0x8
	char* parentList; // 0xC
	short* quats; // 0x10
	float* trans; // 0x14
	char* partClassification; // 0x18
	void* baseMat; // 0x1C
	XSurface* surfaces; // 0x20
	Material** materials; // 0x24
	XModelLodInfo lodInfo[4];
	void* collSurfs;
	int numCollSurfs;
	int contents;
	XBoneInfo* boneInfo;
	float radius;
	float mins[3];
	float maxs[3];
	short numLods;
	short collLod;
	XModelStreamInfo streamInfo;	// is not loaded on ps3
	int memUsage;
	char flags;
	int* physPreset;
	int* physGeoms;
};

struct WeaponDef
{
	const char* szInternalName; // 0x0
	const char* szDisplayName; // 0x4
	int unk001; // 0x8
	XModel* gunXModel[16]; // 0xC
	XModel* handXModel; // 0x4C
	const char* szXAnims[33]; // 0x50
	const char* szModeName; // 0xD4
	char unk002[0x1E4]; // 0xD8
	XModel* worldModel[16]; // 0x2BC
	XModel* worldClipModel; // 0x2FC
	XModel* rocketModel; // 0x300
	XModel* knifeModel; // 0x304
	XModel* worldKnifeModel; // 0x308
};

typedef struct
{
	int infoValid;	 // 0x0
	int nextValid;	 // 0x4
	int clientNum;	 // 0x8
	char name[0x30]; // 0xC
	char body[0x40]; // 0x3C
	char head[0x40]; // 0x7C
	char attachModelNames[6][0x40]; // 0xBC
	char attachTagNames[6][0x40]; // 0x23C
	char unknown[0x110];  // 0x3BC
} clientinfo_t; // 0x4CC

union GfxColor
{
	unsigned int packed;
	char array[4];
};

union PackedTexCoords
{
	uint16_t packed[2];
};

struct GfxWorldVertex {

	float xyz[3]; // 0x0
	float binormalSign; // 0xC
	GfxColor color; // 0x10
	float texCoord[2]; // 0x14
	float lmapCoord[2]; // 0x1C
	PackedUnitVec normal; // 0x24
	PackedUnitVec tangent; // 0x28
};

struct D3DResource
{
	unsigned int Common;
	unsigned int ReferenceCount;
	unsigned int Fence;
	unsigned int ReadFence;
	unsigned int Identifier;
	unsigned int BaseFlush;
};

/* 1291 */
struct $38DAE03C5F32738BE3C3EF16C20C38D7
{
	char gap0[4];
	int _bf4;
};

/* 1292 */
union GPUVERTEX_FETCH_CONSTANT
{
	unsigned int dword[2];
	$38DAE03C5F32738BE3C3EF16C20C38D7 _s1;
};

struct D3DVertexBuffer
{
	D3DResource baseclass_0; // 0x0
	GPUVERTEX_FETCH_CONSTANT Format; // 0x18
}; // size = 0x20

struct GfxWorldVertexData {

	GfxWorldVertex* vertices; // 0x0
	D3DVertexBuffer worldVb; // 0x4
}; // size = 0x24

struct srfTriangles_t {

	int vertexLayerData; // 0x0
	int firstVertex; // 0x4
	unsigned __int16 vertexCount; // 0x8
	unsigned __int16 triCount; // 0xA
	int baseIndex; // 0xC
}; // size 0x10


   /* 6907 */
struct WaterWritable
{
	float floatTime;
};

struct MaterialStreamRouting
{
	char source;
	char dest;
};

struct D3DVertexDeclaration
{
	D3DResource baseclass_0;
};

/* 6890 */
union MaterialVertexStreamRouting
{
	MaterialStreamRouting data[16];
	D3DVertexDeclaration* decl[15];
};

struct MaterialVertexDeclaration
{
	char streamCount;
	char hasOptionalSource;
	__declspec(align(4)) MaterialVertexStreamRouting routing;
};

struct GfxVertexShaderLoadDef
{
	char* cachedPart;
	char* physicalPart;
	unsigned __int16 cachedPartSize;
	unsigned __int16 physicalPartSize;
};

struct D3DVertexShader
{
	D3DResource baseclass_0;
};

/* 6893 */
union MaterialVertexShaderProgram
{
	D3DVertexShader* vs;
	GfxVertexShaderLoadDef loadDef;
};

/* 6894 */
struct MaterialVertexShader
{
	const char* name;
	MaterialVertexShaderProgram prog;
};

struct GfxPixelShaderLoadDef
{
	char* cachedPart;
	char* physicalPart;
	unsigned __int16 cachedPartSize;
	unsigned __int16 physicalPartSize;
};

struct D3DPixelShader
{
	D3DResource baseclass_0;
};

/* 6896 */
union MaterialPixelShaderProgram
{
	D3DPixelShader* ps;
	GfxPixelShaderLoadDef loadDef;
};

/* 6897 */
struct MaterialPixelShader
{
	const char* name;
	MaterialPixelShaderProgram prog;
};

struct MaterialArgumentCodeConst
{
	unsigned __int16 index;
	char firstRow;
	char rowCount;
};

/* 6899 */
union MaterialArgumentDef
{
	const float* literalConst;
	MaterialArgumentCodeConst codeConst;
	unsigned int codeSampler;
	unsigned int nameHash;
};

struct MaterialShaderArgument
{
	unsigned __int16 type;
	unsigned __int16 dest;
	MaterialArgumentDef u;
};

struct GfxSurface {

	srfTriangles_t tris; // 0x0
	Material* material; // 0x10
	char lightmapIndex; // 0x14
	char reflectionProbeIndex; // 0x15
	char primaryLightIndex; // 0x16
	char castsSunShadow; // 0x17
	float bounds[2][3];  // 0x18
}; // size 0x30

struct XModelCollSurf
{
	char* tri; // element size 48
	int numTri;
	char pad[36];
};

struct GfxPackedPlacement
{
	float origin[3]; // 0x0
	float rotation[3][3]; // 0xC
	float scale; // 0x30
}; // size 0x34

struct DObjAnimMat
{
	float quat[4];
	float trans[3];
	float transWeight;
};

struct XModelCollSurf_s
{
	float mins[3];
	float maxs[3];
	int boneIdx;
	int contents;
	int surfFlags;
};

/* 7039 */
struct GfxStaticModelDrawInst
{
	float cullDist; // 0x0
	GfxPackedPlacement placement; // 0x4
	XModel* model; // 0x38
	char padding_0x3C[0x10]; // 0x3C
}; // size 0x4C

struct GfxWorld {

	const char* name; // 0x0
	const char* baseName; // 0x4
	int planeCount; // 0x8
	int nodeCount; // 0xC
	int indexCount; // 0x10
	unsigned __int16* indices; // 0x14
	char padding_0x18[0x10]; // 0x18
	GfxImage* skyImage; // 0x28
	int skySamplerState; // 0x2C
	unsigned int vertexCount; // 0x30
	GfxWorldVertexData vertexData; // 0x34
	char padding_0x38[0x1EC]; // 0x58
	int modelCount; // 0x244
	int surfaceCount; // 0x248
	int something; // 0x24C
	char padding_0x250[0x44]; // 0x250
	GfxSurface* surfaces; // 0x294 - originally part of GfxWorldDpvsStatic
	char padding_0x298[0x4]; // 0x298
	GfxStaticModelDrawInst* models; // 0x29C
};



#pragma region Dvar-related structs
struct DvarStringLimits
{
	std::int32_t stringCount;
	const char** strings;
};

struct DvarIntegerLimits
{
	std::int32_t min;
	std::int32_t max;
};

struct DvarFloatLimits
{
	float min;
	float max;
};

union DvarLimits
{
	DvarStringLimits enumeration;
	DvarIntegerLimits integer;
	DvarFloatLimits value;
	DvarFloatLimits vector;
};

struct DvarValueStringBuf
{
	const char* pad;
	char string[12];
};

union DvarValue
{
	bool enabled;
	std::int32_t integer;
	uint32_t unsignedInt;
	float value;
	float vector[4];
	const char* string;
	DvarValueStringBuf stringBuf;
	uint8_t color[4];
};

struct dvar_s
{
	char* name;
	const char* description;
	uint16_t flags;
	uint8_t type;
	bool modified;
	DvarValue current;
	DvarValue latched;
	DvarValue reset;
	DvarLimits domain;
	dvar_s* next;
	dvar_s* hashNext;
};
#pragma endregion

typedef int qboolean;

typedef union qfile_gus {
	FILE* o;
	void* z;
} qfile_gut;

typedef struct qfile_us {
	qfile_gut file;
} qfile_ut;

typedef struct {
	qfile_ut handleFiles;
	qboolean handleSync;
	int	fileSize;
	int	zipFilePos;
	int	zipFileLen;
	qboolean zipFile;
	qboolean streamed;
	char name[256];
} fileHandleData_t;
