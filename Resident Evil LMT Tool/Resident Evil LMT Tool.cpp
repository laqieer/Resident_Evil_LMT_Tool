// Resident Evil LMT Tool.cpp : 定义控制台应用程序的入口点。
// 生化危机动作文件.lmt的mod辅助工具
// by 拉切尔
// 2018-05-09

#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

// 读取LMT文件结构
uint32_t *readLMT(char *LMTFilename, int *magicNum, short *version, short *animationNum)
{
	ifstream lmt(LMTFilename, ios::in | ios::binary);
	if (lmt)
	{
		lmt.read((char *)magicNum, 4);
		if (*magicNum != 0x544d4c)
		{
			cout << "Unknown input file format: " << *magicNum << endl;
			lmt.close();
			return nullptr;
		}

		lmt.read((char *)version, 2);

		lmt.read((char *)animationNum, 2);

		uint32_t *animationOffset = new uint32_t[*animationNum];
		lmt.read((char *)animationOffset, *animationNum * 4);
		lmt.close();
		return animationOffset;
	}
	else
	{
		cout << "Cannot open file: " << LMTFilename << endl;
		return nullptr;
	}
}

// 计算动画大小
int getAnimationSize(int animationID, uint32_t animationOffset[], short animationNum, int filesize)
{
	uint32_t animationBegin = animationOffset[animationID];
	uint32_t animationEnd = 0;

	for (int i = 0; i < animationNum; i++)
	{
		if (animationOffset[i] > animationBegin && (animationEnd == 0 || animationOffset[i] < animationEnd))
		{
			animationEnd = animationOffset[i];
		}
	}

	if (!animationEnd)
	{
		animationEnd = filesize;
	}
	int animationSize = animationEnd - animationBegin;

	return animationSize;
}

// 合并LMT(A<-B)
int mergeLMT(char *LMTFilenameA, char *LMTFilenameB, bool overwite)
{
	int magicNumA, magicNumB;
	short versionA, animationNumA, versionB, animationNumB;
	uint32_t *animationOffsetA = nullptr;
	uint32_t *animationOffsetB = nullptr;

	animationOffsetA = readLMT(LMTFilenameA, &magicNumA, &versionA, &animationNumA);
	animationOffsetB = readLMT(LMTFilenameB, &magicNumB, &versionB, &animationNumB);

	if (animationOffsetA && animationOffsetB)
	{
		ofstream lmtA(LMTFilenameA, ios::ate | ios::in | ios::binary);
		ifstream lmtB(LMTFilenameB, ios::in | ios::binary);

		lmtB.seekg(0, ios::end);
		int filesizeB = lmtB.tellg();

		cout << "Merged animations:" << endl;

		int animationSize;

		for (int i = 0; i < animationNumA; i++)
		{
			if (animationOffsetB[i] && (!animationOffsetA[i] || overwite))
			{
				cout << i << endl;
				animationSize = getAnimationSize(i, animationOffsetB, animationNumB, filesizeB);
				char *buffer = new char[animationSize];
				lmtB.seekg(animationOffsetB[i], ios::beg);
				lmtB.read(buffer, animationSize);
				uint32_t offset = lmtA.tellp();
				cout << offset << endl;
				lmtA.write(buffer, animationSize);
				lmtA.seekp(8 + 4 * i, ios::beg);
				lmtA.write((char *)&offset, 4);
				lmtA.seekp(0, ios::end);
				delete[] buffer;
			}
		}

		lmtA.close();
		lmtB.close();

		delete[] animationOffsetA;
		delete[] animationOffsetB;

		return 1;
	}

	if(animationOffsetA)
		delete[] animationOffsetA;
	if(animationOffsetB)
		delete[] animationOffsetB;

	return 0;
}

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		cout << "Resident Evil LMT File Reader\nby tiki" << endl;
		cout << "读取生化危机动作文件(.lmt)结构" << endl;
		cout << "Usage: [option] LMTFiles > TXTFile" << endl;
		cout << "option: -r\tread struct info from LMT file" << endl;
		cout << "\t-m\tmerge the animations in the second LMT file into the first one (does not overwrite existing animations)" << endl;
		cout << "\t-mo\tmerge the animations in the second LMT file into the first one (overwrite existing animations)" << endl;
	}
	else
	{
		int magicNum; // 幻数("LMT")
		short version; // 版本号
		short animationNum; // 动画数
		uint32_t *animationOffset = nullptr; // 动画数据偏移地址表
		
		if (strcmp(argv[1], "-r") == 0)	// 读取LMT文件结构并输出
		{
			animationOffset = readLMT(argv[2], &magicNum, &version, &animationNum);
			if (animationOffset)
			{
				cout << "LMT File: " << argv[2] << endl;
				cout << "Total number of animations: " << animationNum << endl;
				cout << "ID\tOffset\n";
				for (int i = 0; i < animationNum; i++)
				{
					cout << dec << i << '\t' << hex << animationOffset[i] << endl;
				}
				delete[] animationOffset;
			}
		}
		else
		{
			if (strcmp(argv[1], "-m") == 0) // 整合第二个LMT文件中的动画到第一个LMT文件里(不覆盖第一个LMT里现存的动画)
			{
				if (!mergeLMT(argv[2], argv[3], false))
					cout << "Fail to merge " << argv[3] << " into " << argv[2] << endl;
			}
			else
			{
				if (strcmp(argv[1], "-mo") == 0) // 整合第二个LMT文件中的动画到第一个LMT文件里(覆盖第一个LMT里现存的动画)
				{
					if (!mergeLMT(argv[2], argv[3], true))
						cout << "Fail to merge " << argv[3] << " into " << argv[2] << endl;
				}
				else
				{
					cout << "Unsupported option: " << argv[1] << endl;
				}
			}
		}
	}
	return 0;
}

