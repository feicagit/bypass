 #pragma once
//
//选择性模糊算法
//
#include<windows.h>
#include<sdkddkver.h>
#include<memory.h>
#include<math.h>
#include<stdlib.h>
#include<emmintrin.h>
namespace nim_comp
{
	const float Inv255 =(float)(1.0 / 255);
	const double Eps = 2.220446049250313E-16;

	enum EdgeMode									//	某些领域算法的边界处理方式
	{
		kTile = 0,									//	重复边缘像素
		kSmear = 1									//	镜像边缘像素
	};

	enum IS_RET
	{
		IS_RET_OK,									//	正常
		IS_RET_ERR_OUTOFMEMORY,						//	内存溢出
		IS_RET_ERR_STACKOVERFLOW,					//	堆栈溢出
		IS_RET_ERR_NULLREFERENCE,					//	空引用
		IS_RET_ERR_ARGUMENTOUTOFRANGE,				//	参数不在正常范围内
		IS_RET_ERR_PARAMISMATCH,					//	参数不匹配
		IS_RET_ERR_DIVIDEBYZERO,
		IS_RET_ERR_INDEXOUTOFRANGE,
		IS_RET_ERR_NOTSUPPORTED,
		IS_RET_ERR_OVERFLOW,
		IS_RET_ERR_FILENOTFOUND,
		IS_RET_ERR_UNKNOWN
	};

	enum IS_DEPTH
	{
		IS_DEPTH_8U = 0,			//	unsigned char
		IS_DEPTH_8S = 1,			//	char
		IS_DEPTH_16S = 2,			//	short
		IS_DEPTH_32S = 3,			//  int
		IS_DEPTH_32F = 4,			//	float
		IS_DEPTH_64F = 5,			//	double
	};

	struct TMatrix
	{
		int Width;					//	矩阵的宽度
		int Height;					//	矩阵的高度
		int WidthStep;				//	矩阵一行元素的占用的字节数
		int Channel;				//	矩阵通道数
		int Depth;					//	矩阵元素的类型
		unsigned char *Data;		//	矩阵的数据
		int Reserved;				//	保留使用
	};

	int IS_ELEMENT_SIZE(int Depth);																//	获取元素大小
	void *IS_AllocMemory(unsigned int Size, bool ZeroMemory = false);							//	分配内存，32字节对齐
	void IS_FreeMemory(void *Ptr);																//	释放内存
	IS_RET IS_CreateMatrix(int Width, int Height, int Depth, int Channel, TMatrix **Matrix);	//	创建数据矩阵
	IS_RET IS_FreeMatrix(TMatrix **Matrix);														//	释放数据矩阵
	IS_RET IS_CloneMatrix(TMatrix *Src, TMatrix **Dest);	//	克隆数据矩阵

#define WIDTHBYTES(bytes) (((bytes * 8) + 31) / 32 * 4)
	unsigned char ClampToByte(int Value);
	void HistgramAddShort(unsigned short *X, unsigned short *Y);
	void HistgramSubShort(unsigned short *X, unsigned short *Y);
	void HistgramSubAddShort(unsigned short *X, unsigned short *Y, unsigned short *Z);
	IS_RET GetValidCoordinate(int Width, int Height, int Left, int Right, int Top, int Bottom, EdgeMode Edge, TMatrix **Row, TMatrix **Col);
	IS_RET SplitRGBA(TMatrix *Src, TMatrix **Blue, TMatrix **Green, TMatrix **Red, TMatrix **Alpha);
	IS_RET CombineRGBA(TMatrix *Dest, TMatrix *Blue, TMatrix *Green, TMatrix *Red, TMatrix *Alpha);
	IS_RET __stdcall SelectiveBlur(TMatrix *Src, TMatrix *Dest, int Radius, int Threshold, EdgeMode Edge);
}