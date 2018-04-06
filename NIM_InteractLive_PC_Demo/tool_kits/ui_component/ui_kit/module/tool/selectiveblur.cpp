#include "SelectiveBlur.h"
namespace nim_comp
{
	void *IS_AllocMemory(unsigned int Size, bool ZeroMemory)
	{
		void *Ptr = _mm_malloc(Size, 32);		//	如果是手机或者其他情况，请将此函数换为malloc之类的，在PC上用它主要是实现内存分配的字节对齐。
		if (Ptr != NULL)
			if (ZeroMemory == true)
				memset(Ptr, 0, Size);
		return Ptr;
	}

	void IS_FreeMemory(void *Ptr)
	{
		if (Ptr != NULL) _mm_free(Ptr);
	}

	/// <summary>
	/// 根据矩阵元素的类型来获取一个元素实际占用的字节数
	/// </summary>
	/// <param name="Depth">根据矩阵元素的类型。</param>
	/// <remarks> 只作为内部使用。</remarks>
	int IS_ELEMENT_SIZE(int Depth)
	{
		int Size;
		switch (Depth)
		{
		case IS_DEPTH_8U:
			Size = sizeof(unsigned char);
			break;
		case IS_DEPTH_8S:
			Size = sizeof(char);
			break;
		case IS_DEPTH_16S:
			Size = sizeof(short);
			break;
		case IS_DEPTH_32S:
			Size = sizeof(int);
			break;
		case IS_DEPTH_32F:
			Size = sizeof(float);
			break;
		case IS_DEPTH_64F:
			Size = sizeof(double);
			break;
		default:
			Size = 0;
			break;
		}
		return Size;
	}

	/// <summary>
	/// 创建新的矩阵数据。
	/// </summary>
	/// <param name="Width">矩阵的宽度。</param>
	/// <param name="Height">矩阵的高度。</param>
	/// <param name="Depth">矩阵的元素类型。</param>
	/// <param name="Channel">矩阵的通道数。</param>
	/// <param name="Matrix">返回的矩阵对象。</param>
	/// <returns>返回值为0表示成功，其他表示失败。</returns>
	IS_RET IS_CreateMatrix(int Width, int Height, int Depth, int Channel, TMatrix **Matrix)
	{
		if (Width < 1 || Height < 1) return IS_RET_ERR_ARGUMENTOUTOFRANGE;
		if (Depth != IS_DEPTH_8U && Depth != IS_DEPTH_8S && Depth != IS_DEPTH_16S && Depth != IS_DEPTH_32S && Depth != IS_DEPTH_32F && Depth != IS_DEPTH_64F) return IS_RET_ERR_ARGUMENTOUTOFRANGE;
		if (Channel != 1 && Channel != 2 && Channel != 3 && Channel != 4) return IS_RET_ERR_ARGUMENTOUTOFRANGE;
		*Matrix = (TMatrix *)IS_AllocMemory(sizeof(TMatrix));					//	真的不相信这个函数还有分配不成功的时刻
		(*Matrix)->Width = Width;
		(*Matrix)->Height = Height;
		(*Matrix)->Depth = Depth;
		(*Matrix)->Channel = Channel;
		(*Matrix)->WidthStep = WIDTHBYTES(Width * Channel * IS_ELEMENT_SIZE(Depth));
		(*Matrix)->Data = (unsigned char *)IS_AllocMemory((*Matrix)->Height * (*Matrix)->WidthStep, true);
		if ((*Matrix)->Data == NULL)
		{
			IS_FreeMemory(*Matrix);
			return IS_RET_ERR_OUTOFMEMORY;
		}
		(*Matrix)->Reserved = 0;
		return IS_RET_OK;
	}

	/// <summary>
	/// 释放创建的矩阵。
	/// </summary>
	/// <param name="Matrix">需要释放的矩阵对象。</param>
	/// <returns>返回值为0表示成功，其他表示失败。</returns>
	IS_RET IS_FreeMatrix(TMatrix **Matrix)
	{
		if ((*Matrix) == NULL) return IS_RET_ERR_NULLREFERENCE;
		if ((*Matrix)->Data == NULL)
		{
			IS_FreeMemory((*Matrix));
			return IS_RET_ERR_OUTOFMEMORY;
		}
		else
		{
			IS_FreeMemory((*Matrix)->Data);			//	注意释放的顺序
			IS_FreeMemory((*Matrix));
			return IS_RET_OK;
		}
	}

	/// <summary>
	/// 克隆现有的矩阵。
	/// </summary>
	/// <param name="Src">被克隆的矩阵对象。</param>
	/// <param name="Dest">克隆后的矩阵对象。</param>
	/// <returns>返回值为0表示成功，其他表示失败。</returns>
	IS_RET IS_CloneMatrix(TMatrix *Src, TMatrix **Dest)
	{
		if (Src == NULL) return IS_RET_ERR_NULLREFERENCE;
		if (Src->Data == NULL) return IS_RET_ERR_NULLREFERENCE;
		IS_RET Ret = IS_CreateMatrix(Src->Width, Src->Height, Src->Depth, Src->Channel, Dest);
		if (Ret == IS_RET_OK) memcpy((*Dest)->Data, Src->Data, (*Dest)->Height * (*Dest)->WidthStep);
		return Ret;
	}


	/// <summary>
	/// 将数据截断在Byte数据类型内，开发时间：2014.12.8
	/// </summary>
	/// <param name="Value">需要裁减的原始数据。</param>
	/// <remarks> 参考：http://www.cnblogs.com/zyl910/archive/2012/03/12/noifopex1.html。</remarks>
	/// <remarks> 用位掩码做饱和处理.用带符号右移生成掩码，不要写成宏定义，会因为参数类型而出现问题，且这样写系统会自动内联，效率上也比宏定义高一些。</remarks>

	unsigned char ClampToByte(int Value)
	{
		return ((Value | ((signed int)(255 - Value) >> 31)) & ~((signed int)Value >> 31));
	}


	/// <summary>
	/// 无符号短整形直方图数据相加，Y = X + Y， 整理时间2014.12.28; 
	/// </summary>
	/// <param name="X">加数。</param>
	/// <param name="Y">被加数，结果保存于该数中。</param>
	/// <remarks>使用了SSE优化。</remarks>
	void HistgramAddShort(unsigned short *X, unsigned short *Y)
	{
		*(__m128i*)(Y + 0) = _mm_add_epi16(*(__m128i*)&Y[0], *(__m128i*)&X[0]);		//	不要想着用自己写的汇编超过他的速度了，已经试过了
		*(__m128i*)(Y + 8) = _mm_add_epi16(*(__m128i*)&Y[8], *(__m128i*)&X[8]);
		*(__m128i*)(Y + 16) = _mm_add_epi16(*(__m128i*)&Y[16], *(__m128i*)&X[16]);
		*(__m128i*)(Y + 24) = _mm_add_epi16(*(__m128i*)&Y[24], *(__m128i*)&X[24]);
		*(__m128i*)(Y + 32) = _mm_add_epi16(*(__m128i*)&Y[32], *(__m128i*)&X[32]);
		*(__m128i*)(Y + 40) = _mm_add_epi16(*(__m128i*)&Y[40], *(__m128i*)&X[40]);
		*(__m128i*)(Y + 48) = _mm_add_epi16(*(__m128i*)&Y[48], *(__m128i*)&X[48]);
		*(__m128i*)(Y + 56) = _mm_add_epi16(*(__m128i*)&Y[56], *(__m128i*)&X[56]);
		*(__m128i*)(Y + 64) = _mm_add_epi16(*(__m128i*)&Y[64], *(__m128i*)&X[64]);
		*(__m128i*)(Y + 72) = _mm_add_epi16(*(__m128i*)&Y[72], *(__m128i*)&X[72]);
		*(__m128i*)(Y + 80) = _mm_add_epi16(*(__m128i*)&Y[80], *(__m128i*)&X[80]);
		*(__m128i*)(Y + 88) = _mm_add_epi16(*(__m128i*)&Y[88], *(__m128i*)&X[88]);
		*(__m128i*)(Y + 96) = _mm_add_epi16(*(__m128i*)&Y[96], *(__m128i*)&X[96]);
		*(__m128i*)(Y + 104) = _mm_add_epi16(*(__m128i*)&Y[104], *(__m128i*)&X[104]);
		*(__m128i*)(Y + 112) = _mm_add_epi16(*(__m128i*)&Y[112], *(__m128i*)&X[112]);
		*(__m128i*)(Y + 120) = _mm_add_epi16(*(__m128i*)&Y[120], *(__m128i*)&X[120]);
		*(__m128i*)(Y + 128) = _mm_add_epi16(*(__m128i*)&Y[128], *(__m128i*)&X[128]);
		*(__m128i*)(Y + 136) = _mm_add_epi16(*(__m128i*)&Y[136], *(__m128i*)&X[136]);
		*(__m128i*)(Y + 144) = _mm_add_epi16(*(__m128i*)&Y[144], *(__m128i*)&X[144]);
		*(__m128i*)(Y + 152) = _mm_add_epi16(*(__m128i*)&Y[152], *(__m128i*)&X[152]);
		*(__m128i*)(Y + 160) = _mm_add_epi16(*(__m128i*)&Y[160], *(__m128i*)&X[160]);
		*(__m128i*)(Y + 168) = _mm_add_epi16(*(__m128i*)&Y[168], *(__m128i*)&X[168]);
		*(__m128i*)(Y + 176) = _mm_add_epi16(*(__m128i*)&Y[176], *(__m128i*)&X[176]);
		*(__m128i*)(Y + 184) = _mm_add_epi16(*(__m128i*)&Y[184], *(__m128i*)&X[184]);
		*(__m128i*)(Y + 192) = _mm_add_epi16(*(__m128i*)&Y[192], *(__m128i*)&X[192]);
		*(__m128i*)(Y + 200) = _mm_add_epi16(*(__m128i*)&Y[200], *(__m128i*)&X[200]);
		*(__m128i*)(Y + 208) = _mm_add_epi16(*(__m128i*)&Y[208], *(__m128i*)&X[208]);
		*(__m128i*)(Y + 216) = _mm_add_epi16(*(__m128i*)&Y[216], *(__m128i*)&X[216]);
		*(__m128i*)(Y + 224) = _mm_add_epi16(*(__m128i*)&Y[224], *(__m128i*)&X[224]);
		*(__m128i*)(Y + 232) = _mm_add_epi16(*(__m128i*)&Y[232], *(__m128i*)&X[232]);
		*(__m128i*)(Y + 240) = _mm_add_epi16(*(__m128i*)&Y[240], *(__m128i*)&X[240]);
		*(__m128i*)(Y + 248) = _mm_add_epi16(*(__m128i*)&Y[248], *(__m128i*)&X[248]);
	}


	/// <summary>
	/// 无符号短整形直方图数据相减，Y = Y + X， 整理时间2014.12.28; 
	/// </summary>
	/// <param name="X">减数。</param>
	/// <param name="Y">被减数，结果保存于该数中。</param>
	/// <remarks>使用了SSE优化。</remarks>
	void HistgramSubShort(unsigned short *X, unsigned short *Y)
	{
		*(__m128i*)(Y + 0) = _mm_sub_epi16(*(__m128i*)&Y[0], *(__m128i*)&X[0]);
		*(__m128i*)(Y + 8) = _mm_sub_epi16(*(__m128i*)&Y[8], *(__m128i*)&X[8]);
		*(__m128i*)(Y + 16) = _mm_sub_epi16(*(__m128i*)&Y[16], *(__m128i*)&X[16]);
		*(__m128i*)(Y + 24) = _mm_sub_epi16(*(__m128i*)&Y[24], *(__m128i*)&X[24]);
		*(__m128i*)(Y + 32) = _mm_sub_epi16(*(__m128i*)&Y[32], *(__m128i*)&X[32]);
		*(__m128i*)(Y + 40) = _mm_sub_epi16(*(__m128i*)&Y[40], *(__m128i*)&X[40]);
		*(__m128i*)(Y + 48) = _mm_sub_epi16(*(__m128i*)&Y[48], *(__m128i*)&X[48]);
		*(__m128i*)(Y + 56) = _mm_sub_epi16(*(__m128i*)&Y[56], *(__m128i*)&X[56]);
		*(__m128i*)(Y + 64) = _mm_sub_epi16(*(__m128i*)&Y[64], *(__m128i*)&X[64]);
		*(__m128i*)(Y + 72) = _mm_sub_epi16(*(__m128i*)&Y[72], *(__m128i*)&X[72]);
		*(__m128i*)(Y + 80) = _mm_sub_epi16(*(__m128i*)&Y[80], *(__m128i*)&X[80]);
		*(__m128i*)(Y + 88) = _mm_sub_epi16(*(__m128i*)&Y[88], *(__m128i*)&X[88]);
		*(__m128i*)(Y + 96) = _mm_sub_epi16(*(__m128i*)&Y[96], *(__m128i*)&X[96]);
		*(__m128i*)(Y + 104) = _mm_sub_epi16(*(__m128i*)&Y[104], *(__m128i*)&X[104]);
		*(__m128i*)(Y + 112) = _mm_sub_epi16(*(__m128i*)&Y[112], *(__m128i*)&X[112]);
		*(__m128i*)(Y + 120) = _mm_sub_epi16(*(__m128i*)&Y[120], *(__m128i*)&X[120]);
		*(__m128i*)(Y + 128) = _mm_sub_epi16(*(__m128i*)&Y[128], *(__m128i*)&X[128]);
		*(__m128i*)(Y + 136) = _mm_sub_epi16(*(__m128i*)&Y[136], *(__m128i*)&X[136]);
		*(__m128i*)(Y + 144) = _mm_sub_epi16(*(__m128i*)&Y[144], *(__m128i*)&X[144]);
		*(__m128i*)(Y + 152) = _mm_sub_epi16(*(__m128i*)&Y[152], *(__m128i*)&X[152]);
		*(__m128i*)(Y + 160) = _mm_sub_epi16(*(__m128i*)&Y[160], *(__m128i*)&X[160]);
		*(__m128i*)(Y + 168) = _mm_sub_epi16(*(__m128i*)&Y[168], *(__m128i*)&X[168]);
		*(__m128i*)(Y + 176) = _mm_sub_epi16(*(__m128i*)&Y[176], *(__m128i*)&X[176]);
		*(__m128i*)(Y + 184) = _mm_sub_epi16(*(__m128i*)&Y[184], *(__m128i*)&X[184]);
		*(__m128i*)(Y + 192) = _mm_sub_epi16(*(__m128i*)&Y[192], *(__m128i*)&X[192]);
		*(__m128i*)(Y + 200) = _mm_sub_epi16(*(__m128i*)&Y[200], *(__m128i*)&X[200]);
		*(__m128i*)(Y + 208) = _mm_sub_epi16(*(__m128i*)&Y[208], *(__m128i*)&X[208]);
		*(__m128i*)(Y + 216) = _mm_sub_epi16(*(__m128i*)&Y[216], *(__m128i*)&X[216]);
		*(__m128i*)(Y + 224) = _mm_sub_epi16(*(__m128i*)&Y[224], *(__m128i*)&X[224]);
		*(__m128i*)(Y + 232) = _mm_sub_epi16(*(__m128i*)&Y[232], *(__m128i*)&X[232]);
		*(__m128i*)(Y + 240) = _mm_sub_epi16(*(__m128i*)&Y[240], *(__m128i*)&X[240]);
		*(__m128i*)(Y + 248) = _mm_sub_epi16(*(__m128i*)&Y[248], *(__m128i*)&X[248]);
	}

	/// <summary>
	/// 无符号短整形直方图数据相加减，Z = Z + Y - X， 整理时间2014.12.28; 
	/// </summary>
	/// <param name="X">减数。</param>
	/// <param name="Y">加数。</param>
	/// <param name="Y">被加数，结果保存于该数中。</param>
	/// <remarks>使用了SSE优化，这样比分两次加 和 减 速度要快。</remarks>
	void HistgramSubAddShort(unsigned short *X, unsigned short *Y, unsigned short *Z)
	{
		*(__m128i*)(Z + 0) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[0], *(__m128i*)&Z[0]), *(__m128i*)&X[0]);						//	不要想着用自己写的汇编超过他的速度了，已经试过了
		*(__m128i*)(Z + 8) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[8], *(__m128i*)&Z[8]), *(__m128i*)&X[8]);
		*(__m128i*)(Z + 16) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[16], *(__m128i*)&Z[16]), *(__m128i*)&X[16]);
		*(__m128i*)(Z + 24) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[24], *(__m128i*)&Z[24]), *(__m128i*)&X[24]);
		*(__m128i*)(Z + 32) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[32], *(__m128i*)&Z[32]), *(__m128i*)&X[32]);
		*(__m128i*)(Z + 40) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[40], *(__m128i*)&Z[40]), *(__m128i*)&X[40]);
		*(__m128i*)(Z + 48) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[48], *(__m128i*)&Z[48]), *(__m128i*)&X[48]);
		*(__m128i*)(Z + 56) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[56], *(__m128i*)&Z[56]), *(__m128i*)&X[56]);
		*(__m128i*)(Z + 64) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[64], *(__m128i*)&Z[64]), *(__m128i*)&X[64]);
		*(__m128i*)(Z + 72) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[72], *(__m128i*)&Z[72]), *(__m128i*)&X[72]);
		*(__m128i*)(Z + 80) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[80], *(__m128i*)&Z[80]), *(__m128i*)&X[80]);
		*(__m128i*)(Z + 88) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[88], *(__m128i*)&Z[88]), *(__m128i*)&X[88]);
		*(__m128i*)(Z + 96) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[96], *(__m128i*)&Z[96]), *(__m128i*)&X[96]);
		*(__m128i*)(Z + 104) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[104], *(__m128i*)&Z[104]), *(__m128i*)&X[104]);
		*(__m128i*)(Z + 112) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[112], *(__m128i*)&Z[112]), *(__m128i*)&X[112]);
		*(__m128i*)(Z + 120) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[120], *(__m128i*)&Z[120]), *(__m128i*)&X[120]);
		*(__m128i*)(Z + 128) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[128], *(__m128i*)&Z[128]), *(__m128i*)&X[128]);
		*(__m128i*)(Z + 136) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[136], *(__m128i*)&Z[136]), *(__m128i*)&X[136]);
		*(__m128i*)(Z + 144) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[144], *(__m128i*)&Z[144]), *(__m128i*)&X[144]);
		*(__m128i*)(Z + 152) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[152], *(__m128i*)&Z[152]), *(__m128i*)&X[152]);
		*(__m128i*)(Z + 160) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[160], *(__m128i*)&Z[160]), *(__m128i*)&X[160]);
		*(__m128i*)(Z + 168) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[168], *(__m128i*)&Z[168]), *(__m128i*)&X[168]);
		*(__m128i*)(Z + 176) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[176], *(__m128i*)&Z[176]), *(__m128i*)&X[176]);
		*(__m128i*)(Z + 184) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[184], *(__m128i*)&Z[184]), *(__m128i*)&X[184]);
		*(__m128i*)(Z + 192) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[192], *(__m128i*)&Z[192]), *(__m128i*)&X[192]);
		*(__m128i*)(Z + 200) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[200], *(__m128i*)&Z[200]), *(__m128i*)&X[200]);
		*(__m128i*)(Z + 208) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[208], *(__m128i*)&Z[208]), *(__m128i*)&X[208]);
		*(__m128i*)(Z + 216) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[216], *(__m128i*)&Z[216]), *(__m128i*)&X[216]);
		*(__m128i*)(Z + 224) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[224], *(__m128i*)&Z[224]), *(__m128i*)&X[224]);
		*(__m128i*)(Z + 232) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[232], *(__m128i*)&Z[232]), *(__m128i*)&X[232]);
		*(__m128i*)(Z + 240) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[240], *(__m128i*)&Z[240]), *(__m128i*)&X[240]);
		*(__m128i*)(Z + 248) = _mm_sub_epi16(_mm_add_epi16(*(__m128i*)&Y[248], *(__m128i*)&Z[248]), *(__m128i*)&X[248]);
	}



	/// <summary>
	/// 按照指定的边缘模式计算扩展后各坐标的有理值
	/// </summary>
	/// <param name="Width">矩阵的宽度。</param>
	/// <param name="Height">矩阵的高度。</param>
	/// <param name="Left">左侧需要扩展的坐标。</param>
	/// <param name="Right">右侧需要扩展的坐标。</param>
	/// <param name="Top">顶部需要扩展的坐标。</param>
	/// <param name="Bottom">底部需要扩展的坐标。</param>
	/// <param name="Edge">处理边缘的方式。</param>
	/// <param name="RowPos">保存行方向的有理坐标值。</param>
	/// <param name="ColPos">保存列方向的有理坐标值。</param>
	/// <returns>返回函数是否执行成功。</returns>
	IS_RET GetValidCoordinate(int Width, int Height, int Left, int Right, int Top, int Bottom, EdgeMode Edge, TMatrix **Row, TMatrix **Col)
	{
		if ((Left < 0) || (Right < 0) || (Top < 0) || (Bottom < 0)) return IS_RET_ERR_ARGUMENTOUTOFRANGE;
		IS_RET Ret = IS_CreateMatrix(Width + Left + Right, 1, IS_DEPTH_32S, 1, Row);
		if (Ret != IS_RET_OK) return Ret;
		Ret = IS_CreateMatrix(1, Height + Top + Bottom, IS_DEPTH_32S, 1, Col);
		if (Ret != IS_RET_OK) return Ret;

		int X, Y, XX, YY, *RowPos = (int *)(*Row)->Data, *ColPos = (int *)(*Col)->Data;

		for (X = -Left; X < Width + Right; X++)
		{
			if (X < 0)
			{
				if (Edge == EdgeMode::kTile)							//重复边缘像素
					RowPos[X + Left] = 0;
				else
				{
					XX = -X;
					while (XX >= Width) XX -= Width;			// 做镜像数据
					RowPos[X + Left] = XX;
				}
			}
			else if (X >= Width)
			{
				if (Edge == EdgeMode::kTile)
					RowPos[X + Left] = Width - 1;
				else
				{
					XX = Width - (X - Width + 2);
					while (XX < 0) XX += Width;
					RowPos[X + Left] = XX;
				}
			}
			else
			{
				RowPos[X + Left] = X;
			}
		}

		for (Y = -Top; Y < Height + Bottom; Y++)
		{
			if (Y < 0)
			{
				if (Edge == EdgeMode::kTile)
					ColPos[Y + Top] = 0;
				else
				{
					YY = -Y;
					while (YY >= Height) YY -= Height;
					ColPos[Y + Top] = YY;
				}
			}
			else if (Y >= Height)
			{
				if (Edge == EdgeMode::kTile)
					ColPos[Y + Top] = Height - 1;
				else
				{
					YY = Height - (Y - Height + 2);
					while (YY < 0) YY += Height;
					ColPos[Y + Top] = YY;
				}
			}
			else
			{
				ColPos[Y + Top] = Y;
			}
		}
		return IS_RET_OK;
	}


	/// <summary>
	/// 将彩色图像分解为R、G、B、A单通道的图像
	/// </summary>
	/// <param name="Src">需要处理的源图像的数据结构。</param>
	/// <param name="Blue">保存Blue通道图像的数据结构。</param>
	/// <param name="Green">保存Green通道图像的数据结构。</param>
	/// <param name="Red">保存Red通道图像的数据结构。</param>
	/// <param name="Alpha">保存Alpha通道图像的数据结构。</param>
	/// <remarks>采用了8位并行处理，速度大约能提高20%。</remarks>
	IS_RET SplitRGBA(TMatrix *Src, TMatrix **Blue, TMatrix **Green, TMatrix **Red, TMatrix **Alpha)
	{
		if (Src == NULL) return IS_RET_ERR_NULLREFERENCE;
		if (Src->Data == NULL) return IS_RET_ERR_NULLREFERENCE;
		if (Src->Depth != IS_DEPTH_8U) return IS_RET_ERR_NOTSUPPORTED;

		IS_RET Ret;
		Ret = IS_CreateMatrix(Src->Width, Src->Height, Src->Depth, 1, Blue);
		if (Ret != IS_RET_OK) goto Done;
		Ret = IS_CreateMatrix(Src->Width, Src->Height, Src->Depth, 1, Green);
		if (Ret != IS_RET_OK) goto Done;
		Ret = IS_CreateMatrix(Src->Width, Src->Height, Src->Depth, 1, Red);
		if (Ret != IS_RET_OK) goto Done;
		if (Src->Channel == 4)
		{
			Ret = IS_CreateMatrix(Src->Width, Src->Height, Src->Depth, 1, Alpha);
			if (Ret != IS_RET_OK) goto Done;
		}

		int X, Y, Block, Width = Src->Width, Height = Src->Height;
		unsigned char *LinePS, *LinePB, *LinePG, *LinePR, *LinePA;
		const int BlockSize = 8;
		Block = Width / BlockSize;						//	8路并行,再多开几路，速度并没有增加了

		if (Src->Channel == 3)
		{
			for (Y = 0; Y < Height; Y++)
			{
				LinePS = Src->Data + Y * Src->WidthStep;
				LinePB = (*Blue)->Data + Y * (*Blue)->WidthStep;
				LinePG = (*Green)->Data + Y * (*Green)->WidthStep;
				LinePR = (*Red)->Data + Y * (*Red)->WidthStep;
				for (X = 0; X < Block * BlockSize; X += BlockSize)			//	如果把LinePB全写在一起，速度反而慢一些
				{
					LinePB[0] = LinePS[0];		LinePG[0] = LinePS[1];		LinePR[0] = LinePS[2];
					LinePB[1] = LinePS[3];		LinePG[1] = LinePS[4];		LinePR[1] = LinePS[5];
					LinePB[2] = LinePS[6];		LinePG[2] = LinePS[7];		LinePR[2] = LinePS[8];
					LinePB[3] = LinePS[9];		LinePG[3] = LinePS[10];		LinePR[3] = LinePS[11];
					LinePB[4] = LinePS[12];		LinePG[4] = LinePS[13];		LinePR[4] = LinePS[14];
					LinePB[5] = LinePS[15];		LinePG[5] = LinePS[16];		LinePR[5] = LinePS[17];
					LinePB[6] = LinePS[18];		LinePG[6] = LinePS[19];		LinePR[6] = LinePS[20];
					LinePB[7] = LinePS[21];		LinePG[7] = LinePS[22];		LinePR[7] = LinePS[23];
					LinePB += 8;				LinePG += 8;				LinePR += 8;				LinePS += 24;
				}
				while (X < Width)
				{
					LinePB[0] = LinePS[0];		LinePG[0] = LinePS[1];		LinePR[0] = LinePS[2];
					LinePB++;					LinePG++;					LinePR++;					LinePS += 3;
					X++;
				}
			}
		}
		else if (Src->Channel == 4)
		{
			for (Y = 0; Y < Height; Y++)
			{
				LinePS = Src->Data + Y * Src->WidthStep;
				LinePB = (*Blue)->Data + Y * (*Blue)->WidthStep;
				LinePG = (*Green)->Data + Y * (*Green)->WidthStep;
				LinePR = (*Red)->Data + Y * (*Red)->WidthStep;
				LinePA = (*Alpha)->Data + Y * (*Alpha)->WidthStep;
				for (X = 0; X < Block * BlockSize; X += BlockSize)
				{
					LinePB[0] = LinePS[0];		LinePG[0] = LinePS[1];		LinePR[0] = LinePS[2];		LinePA[0] = LinePS[3];
					LinePB[1] = LinePS[4];		LinePG[1] = LinePS[5];		LinePR[1] = LinePS[6];		LinePA[1] = LinePS[7];
					LinePB[2] = LinePS[8];		LinePG[2] = LinePS[9];		LinePR[2] = LinePS[10];		LinePA[2] = LinePS[11];
					LinePB[3] = LinePS[12];		LinePG[3] = LinePS[13];		LinePR[3] = LinePS[14];		LinePA[3] = LinePS[15];
					LinePB[4] = LinePS[16];		LinePG[4] = LinePS[17];		LinePR[4] = LinePS[18];		LinePA[4] = LinePS[19];
					LinePB[5] = LinePS[20];		LinePG[5] = LinePS[21];		LinePR[5] = LinePS[22];		LinePA[5] = LinePS[23];
					LinePB[6] = LinePS[24];		LinePG[6] = LinePS[25];		LinePR[6] = LinePS[26];		LinePA[6] = LinePS[27];
					LinePB[7] = LinePS[28];		LinePG[7] = LinePS[29];		LinePR[7] = LinePS[30];		LinePA[7] = LinePS[31];
					LinePB += 8;				LinePG += 8;				LinePR += 8;				LinePA += 8;				LinePS += 32;
				}
				while (X < Width)
				{
					LinePB[0] = LinePS[0];		LinePG[0] = LinePS[1];		LinePR[0] = LinePS[2];		LinePA[0] = LinePS[3];
					LinePB++;					LinePG++;					LinePR++;					LinePA++;					LinePS += 4;
					X++;
				}
			}
		}
		return IS_RET_OK;
	Done:
		if (*Blue != NULL) IS_FreeMatrix(Blue);
		if (*Green != NULL) IS_FreeMatrix(Green);
		if (*Red != NULL) IS_FreeMatrix(Red);
		if (*Alpha != NULL) IS_FreeMatrix(Alpha);
		return Ret;
	}

	/// <summary>
	/// 将R、G、B、A单通道的图像合并为彩色的图像。
	/// </summary>
	/// <param name="Dest">合并处理后的图像的数据结构。</param>
	/// <param name="Blue">Blue通道图像的数据结构。</param>
	/// <param name="Green">Green通道图像的数据结构。</param>
	/// <param name="Red">Red通道图像的数据结构。</param>
	/// <param name="Alpha">Alpha通道图像的数据结构。</param>
	/// <remarks>采用了8位并行处理，速度大约能提高20%。</remarks>
	IS_RET CombineRGBA(TMatrix *Dest, TMatrix *Blue, TMatrix *Green, TMatrix *Red, TMatrix *Alpha)
	{
		if (Dest == NULL || Blue == NULL || Green == NULL || Red == NULL) return IS_RET_ERR_NULLREFERENCE;
		if (Dest->Data == NULL || Blue->Data == NULL || Green->Data == NULL || Red->Data == NULL) return IS_RET_ERR_NULLREFERENCE;
		if ((Dest->Channel != 3 && Dest->Channel != 4) || Blue->Channel != 1 || Green->Channel != 1 || Red->Channel != 1) return IS_RET_ERR_PARAMISMATCH;
		if (Dest->Width != Blue->Width || Dest->Width != Green->Width || Dest->Width != Red->Width || Dest->Width != Blue->Width)  return IS_RET_ERR_PARAMISMATCH;
		if (Dest->Height != Blue->Height || Dest->Height != Green->Height || Dest->Height != Red->Height || Dest->Height != Blue->Height)  return IS_RET_ERR_PARAMISMATCH;

		if (Dest->Channel == 4)
		{
			if (Alpha == NULL) return IS_RET_ERR_NULLREFERENCE;
			if (Alpha->Data == NULL) return IS_RET_ERR_NULLREFERENCE;
			if (Alpha->Channel != 1) return IS_RET_ERR_PARAMISMATCH;
			if (Dest->Width != Alpha->Width || Dest->Height != Alpha->Height) return IS_RET_ERR_PARAMISMATCH;
		}

		int X, Y, Block, Width = Dest->Width, Height = Dest->Height;
		unsigned char *LinePD, *LinePB, *LinePG, *LinePR, *LinePA;
		const int BlockSize = 8;
		Block = Width / BlockSize;						//	8路并行,再多开几路，速度并没有增加了

		if (Dest->Channel == 3)
		{
			for (Y = 0; Y < Height; Y++)
			{
				LinePD = Dest->Data + Y * Dest->WidthStep;
				LinePB = Blue->Data + Y * Blue->WidthStep;
				LinePG = Green->Data + Y * Green->WidthStep;
				LinePR = Red->Data + Y * Red->WidthStep;
				for (X = 0; X < Block * BlockSize; X += BlockSize)				//	如果把LinePB全写在一起，速度区别不大
				{
					LinePD[0] = LinePB[0];		LinePD[1] = LinePG[0];		LinePD[2] = LinePR[0];
					LinePD[3] = LinePB[1];		LinePD[4] = LinePG[1];		LinePD[5] = LinePR[1];
					LinePD[6] = LinePB[2];		LinePD[7] = LinePG[2];		LinePD[8] = LinePR[2];
					LinePD[9] = LinePB[3];		LinePD[10] = LinePG[3];		LinePD[11] = LinePR[3];
					LinePD[12] = LinePB[4];		LinePD[13] = LinePG[4];		LinePD[14] = LinePR[4];
					LinePD[15] = LinePB[5];		LinePD[16] = LinePG[5];		LinePD[17] = LinePR[5];
					LinePD[18] = LinePB[6];		LinePD[19] = LinePG[6];		LinePD[20] = LinePR[6];
					LinePD[21] = LinePB[7];		LinePD[22] = LinePG[7];		LinePD[23] = LinePR[7];
					LinePB += 8;				LinePG += 8;				LinePR += 8;				LinePD += 24;
				}
				while (X < Width)
				{
					LinePD[0] = LinePB[0];		LinePD[1] = LinePG[0];		LinePD[2] = LinePR[0];
					LinePB++;					LinePG++;					LinePR++;					LinePD += 3;
					X++;
				}
			}
		}
		else if (Dest->Channel == 4)
		{
			for (Y = 0; Y < Height; Y++)
			{
				LinePD = Dest->Data + Y * Dest->WidthStep;
				LinePB = Blue->Data + Y * Blue->WidthStep;
				LinePG = Green->Data + Y * Green->WidthStep;
				LinePR = Red->Data + Y * Red->WidthStep;
				LinePA = Alpha->Data + Y * Alpha->WidthStep;
				for (X = 0; X < Block * BlockSize; X += BlockSize)
				{
					LinePD[0] = LinePB[0];		LinePD[1] = LinePG[0];		LinePD[2] = LinePR[0];		LinePD[3] = LinePA[0];
					LinePD[4] = LinePB[1];		LinePD[5] = LinePG[1];		LinePD[6] = LinePR[1];		LinePD[7] = LinePA[1];
					LinePD[8] = LinePB[2];		LinePD[9] = LinePG[2];		LinePD[10] = LinePR[2];		LinePD[11] = LinePA[2];
					LinePD[12] = LinePB[3];		LinePD[13] = LinePG[3];		LinePD[14] = LinePR[3];		LinePD[15] = LinePA[3];
					LinePD[16] = LinePB[4];		LinePD[17] = LinePG[4];		LinePD[18] = LinePR[4];		LinePD[19] = LinePA[4];
					LinePD[20] = LinePB[5];		LinePD[21] = LinePG[5];		LinePD[22] = LinePR[5];		LinePD[23] = LinePA[5];
					LinePD[24] = LinePB[6];		LinePD[25] = LinePG[6];		LinePD[26] = LinePR[6];		LinePD[27] = LinePA[6];
					LinePD[28] = LinePB[7];		LinePD[29] = LinePG[7];		LinePD[30] = LinePR[7];		LinePD[31] = LinePA[7];
					LinePB += 8;				LinePG += 8;				LinePR += 8;				LinePA += 8;				LinePD += 32;
				}
				while (X < Width)
				{
					LinePD[0] = LinePB[0];		LinePD[1] = LinePG[0];		LinePD[2] = LinePR[0];		LinePD[3] = LinePA[0];
					LinePB++;					LinePG++;					LinePD++;					LinePA++;					LinePD += 4;
					X++;
				}
			}
		}
		return IS_RET_OK;
	}


	void Calc(unsigned short *Hist, int Intensity, unsigned char *&Pixel, int Threshold)
	{
		int K, Low, High, Sum = 0, Weight = 0;
		Low = Intensity - Threshold; High = Intensity + Threshold;
		if (Low < 0) Low = 0;
		if (High > 255) High = 255;
		for (K = Low; K <= High; K++)
		{
			Weight += Hist[K];
			Sum += Hist[K] * K;
		}
		if (Weight != 0) *Pixel = Sum / Weight;
	}

	/// <summary>     
	/// 实现图像选择性图像模糊效果，O(1)复杂度,最新整理时间 2014.10.25。
	/// </summary>
	/// <param name="Src">需要处理的源图像的数据结构。</param>
	/// <param name="Dest">保存处理后的图像的数据结构。</param>
	/// <param name="Radius">指定模糊取样区域的大小，有效范围[1,127]。</param>
	/// <param name="Threshold">选项控制相邻像素色调值与中心像素值相差多大时才能成为模糊的一部分,色调值差小于阈值的像素被排除在模糊之外，有效范围[2,255]。</param> 
	IS_RET __stdcall SelectiveBlur(TMatrix *Src, TMatrix *Dest, int Radius, int Threshold, EdgeMode Edge)
	{
		if (Src == NULL || Dest == NULL) return IS_RET_ERR_NULLREFERENCE;
		if (Src->Data == NULL || Dest->Data == NULL) return IS_RET_ERR_NULLREFERENCE;
		if (Src->Width != Dest->Width || Src->Height != Dest->Height || Src->Channel != Dest->Channel || Src->Depth != Dest->Depth || Src->WidthStep != Dest->WidthStep) return IS_RET_ERR_PARAMISMATCH;
		if (Src->Depth != IS_DEPTH_8U || Dest->Depth != IS_DEPTH_8U) return IS_RET_ERR_NOTSUPPORTED;
		if (Radius < 0 || Radius >= 127 || Threshold < 2 || Threshold > 255) return IS_RET_ERR_ARGUMENTOUTOFRANGE;

		IS_RET Ret = IS_RET_OK;

		if (Src->Data == Dest->Data)
		{
			TMatrix *Clone = NULL;
			Ret = IS_CloneMatrix(Src, &Clone);
			if (Ret != IS_RET_OK) return Ret;
			Ret = SelectiveBlur(Clone, Dest, Radius, Threshold, Edge);
			IS_FreeMatrix(&Clone);
			return Ret;
		}
		if (Src->Channel == 1)
		{
			TMatrix *Row = NULL, *Col = NULL;
			unsigned char *LinePS, *LinePD;
			int X, Y, K, Width = Src->Width, Height = Src->Height;
			int *RowOffset, *ColOffSet;

			unsigned short *ColHist = (unsigned short *)IS_AllocMemory(256 * (Width + 2 * Radius) * sizeof(unsigned short), true);
			if (ColHist == NULL) { Ret = IS_RET_ERR_OUTOFMEMORY; goto Done8; }
			unsigned short *Hist = (unsigned short *)IS_AllocMemory(256 * sizeof(unsigned short), true);
			if (Hist == NULL) { Ret = IS_RET_ERR_OUTOFMEMORY; goto Done8; }

			Ret = GetValidCoordinate(Width, Height, Radius, Radius, Radius, Radius, Edge, &Row, &Col);		//	获取坐标偏移量
			if (Ret != IS_RET_OK) goto Done8;

			ColHist += Radius * 256;		RowOffset = ((int *)Row->Data) + Radius;		ColOffSet = ((int *)Col->Data) + Radius;		    	//	进行偏移以便操作

			for (Y = 0; Y < Height; Y++)
			{
				if (Y == 0)											//	第一行的列直方图,要重头计算
				{
					for (K = -Radius; K <= Radius; K++)
					{
						LinePS = Src->Data + ColOffSet[K] * Src->WidthStep;
						for (X = -Radius; X < Width + Radius; X++)
						{
							ColHist[X * 256 + LinePS[RowOffset[X]]]++;
						}
					}
				}
				else												//	其他行的列直方图，更新就可以了
				{
					LinePS = Src->Data + ColOffSet[Y - Radius - 1] * Src->WidthStep;
					for (X = -Radius; X < Width + Radius; X++)		// 删除移出范围内的那一行的直方图数据
					{
						ColHist[X * 256 + LinePS[RowOffset[X]]]--;
					}

					LinePS = Src->Data + ColOffSet[Y + Radius] * Src->WidthStep;
					for (X = -Radius; X < Width + Radius; X++)		// 增加进入范围内的那一行的直方图数据
					{
						ColHist[X * 256 + LinePS[RowOffset[X]]]++;
					}

				}

				memset(Hist, 0, 256 * sizeof(unsigned short));		//	每一行直方图数据清零先

				LinePS = Src->Data + Y * Src->WidthStep;
				LinePD = Dest->Data + Y * Dest->WidthStep;

				for (X = 0; X < Width; X++)
				{
					if (X == 0)
					{
						for (K = -Radius; K <= Radius; K++)			//	行第一个像素，需要重新计算	
							HistgramAddShort(ColHist + K * 256, Hist);
					}
					else
					{
						/*	HistgramAddShort(ColHist + RowOffset[X + Radius] * 256, Hist);
						HistgramSubShort(ColHist + RowOffset[X - Radius - 1] * 256, Hist);
						*/
						HistgramSubAddShort(ColHist + RowOffset[X - Radius - 1] * 256, ColHist + RowOffset[X + Radius] * 256, Hist);  //	行内其他像素，依次删除和增加就可以了
					}
					Calc(Hist, LinePS[0], LinePD, Threshold);

					LinePS++;
					LinePD++;
				}
			}
			ColHist -= Radius * 256;		//	恢复偏移操作
		Done8:
			IS_FreeMatrix(&Row);
			IS_FreeMatrix(&Col);
			IS_FreeMemory(ColHist);
			IS_FreeMemory(Hist);

			return Ret;
		}
		else
		{
			TMatrix *Blue = NULL, *Green = NULL, *Red = NULL, *Alpha = NULL;			//	由于C变量如果不初始化，其值是随机值，可能会导致释放时的错误。
			IS_RET Ret = SplitRGBA(Src, &Blue, &Green, &Red, &Alpha);
			if (Ret != IS_RET_OK) goto Done24;
			Ret = SelectiveBlur(Blue, Blue, Radius, Threshold, Edge);
			if (Ret != IS_RET_OK) goto Done24;
			Ret = SelectiveBlur(Green, Green, Radius, Threshold, Edge);
			if (Ret != IS_RET_OK) goto Done24;
			Ret = SelectiveBlur(Red, Red, Radius, Threshold, Edge);
			if (Ret != IS_RET_OK) goto Done24;											//	32位的Alpha不做任何处理，实际上32位的相关算法基本上是不能分通道处理的
			Ret = CombineRGBA(Dest, Blue, Green, Red, Alpha);
		Done24:
			IS_FreeMatrix(&Blue);
			IS_FreeMatrix(&Green);
			IS_FreeMatrix(&Red);
			IS_FreeMatrix(&Alpha);
			return Ret;
		}
	}
}