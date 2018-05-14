// Resident Evil LMT Tool.cpp : �������̨Ӧ�ó������ڵ㡣
// ����Σ�������ļ�.lmt��mod��������
// by ���ж�
// 2018-05-09

#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

// ȡ����������Сֵ
#define min(x,y)	((x)<(y)?(x):(y))

// ��ȡLMT�ļ��ṹ
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

// �����ļ�
int jointFiles(ofstream &fileA, ifstream &fileB)
{
	fileB.seekg(0, ios::end);
	streamoff filesizeB = fileB.tellg();
	char *buffer = new char[filesizeB];
	fileB.seekg(0, ios::beg);
	fileB.read(buffer, filesizeB);
	fileA.seekp(0, ios::end);
	fileA.write(buffer, filesizeB);
	delete[] buffer;
	return 1;
}

// ָ��
struct Pointer
{
	streamoff self;	// �Լ���ƫ�Ƶ�ַ
	streamoff target; // ָ���ƫ�Ƶ�ַ
};

// ɨ��LMT��ָ��
int scanPointersInLMT(ifstream &lmt, vector<Pointer>&pointers)
{
	// ��ȡ��������
	short animationNum;
	lmt.seekg(6, ios::beg);
	lmt.read((char *)&animationNum, 2);

	// ��ȡ����ͷƫ�Ʊ�
	uint32_t *animationOffset = new uint32_t[animationNum];
	lmt.seekg(8, ios::beg);
	lmt.read((char *)animationOffset, animationNum * 4);
	
	// ����ͷ0x40�ֽڣ�����3��ָ��
	for (int i = 0; i < animationNum; i++)
	{
		if (animationOffset[i])
		{
			// cout << dec << i << "\t--";

			lmt.seekg(animationOffset[i], ios::beg);
			Pointer point = { animationOffset[i], 0 };
			lmt.read((char *)&point.target, 4);
			pointers.push_back(point);

			// cout << hex << point.self << ":" << point.target << endl;

			int boneCount = 1;
			lmt.seekg(animationOffset[i] + 4, ios::beg);
			lmt.read((char *)&boneCount, 4);
			// cout << dec << i << hex << "\t0x" << animationOffset[i] <<"\tboneCount = " << dec << boneCount << endl;
			if (boneCount < 1)
			{
				boneCount = 1;
			}

			// ����ͷ�еĵ�һ��ָ��ָ��ؼ�֡����ṹ�����飬�ýṹ�峤��Ϊ0x24������2��ָ��
			if (point.target)
			{
				for (int j = 0; j < boneCount; j++)
				{
					// bufferSize����ָ��
					/*Pointer pBufferSize = { point.target + 8 + 0x24 * j, 0 };
					lmt.seekg(pBufferSize.self, ios::beg);
					lmt.read((char *)&pBufferSize.target, 4);
					pointers.push_back(pBufferSize);

					cout << "\t\t--" << hex << pBufferSize.self << ":" << pBufferSize.target << endl;*/

					Pointer pBufferOffset = { point.target + 12 + 0x24 * j, 0 };
					lmt.seekg(pBufferOffset.self, ios::beg);
					lmt.read((char *)&pBufferOffset.target, 4);
					pointers.push_back(pBufferOffset);

					// cout << "\t\t--" << hex << pBufferOffset.self << ":" << pBufferOffset.target << endl;

					Pointer pBoundsOffset = { point.target + 32 + 0x24 * j, 0 };
					lmt.seekg(pBoundsOffset.self, ios::beg);
					lmt.read((char *)&pBoundsOffset.target, 4);
					pointers.push_back(pBoundsOffset);

					// cout << "\t\t--" << hex << pBoundsOffset.self << ":" << pBoundsOffset.target << endl << endl;
				}
			}

			point.self += 0x34;
			lmt.seekg(point.self, ios::beg);
			lmt.read((char *)&point.target, 4);
			pointers.push_back(point);

			// cout << "\t--" << hex << point.self << ":" << point.target << endl;

			// ��2��ָ��ָ��Ľṹ������,�ṹ�峤��0x48,���4���ֽ���ָ��,���鳤��4
			if (point.target)
			{
				for (int j = 0; j < 4; j++)
				{
					Pointer p = { point.target + 0x44 + 0x48 * j, 0 };
					lmt.seekg(p.self, ios::beg);
					lmt.read((char *)&p.target, 4);
					pointers.push_back(p);
					// cout << "\t\t--" << hex << p.self << ":" << p.target << endl;
				}
			}

			point.self += 4;
			lmt.seekg(point.self, ios::beg);
			lmt.read((char *)&point.target, 4);
			pointers.push_back(point);

			// cout << "\t--" << hex << point.self << ":" << point.target << endl;

			// ��3��ָ��ָ��Ľṹ������,�ṹ�峤��12,���4���ֽ���ָ��,���鳤��4
			if (point.target)
			{
				for (int j = 0; j < 4; j++)
				{
					Pointer p = { point.target + 8 + 12 * j, 0 };
					lmt.seekg(p.self, ios::beg);
					lmt.read((char *)&p.target, 4);
					pointers.push_back(p);
					// cout << "\t\t--" << hex << p.self << ":" << p.target << endl;
				}
			}
		}
	}

	return 1;
}

// ��дLMT��ָ��
int updatePointersInLMT(ofstream &lmt, vector<Pointer>&pointers, streamoff offset)
{
	if (pointers.empty() || offset == 0)
	{
		return 0;
	}
	
	vector<Pointer>::iterator it;
	/*for (it = pointers.begin(); it != pointers.end(); it++)
	{
		cout << "0x" << hex << it->self << "\t->\t0x" << it->target << endl;
	}*/
	for (it = pointers.begin(); it != pointers.end(); it++)
	{
		if (it->target)
		{
			lmt.seekp(it->self + offset, ios::beg);
			streamoff newPtr = it->target + offset;
			lmt.write((char *)&newPtr, 4);
		}
	}
	return 1;
}

// �ϲ�LMT(A<-B)
int mergeLMT(char *LMTFilenameA, char *LMTFilenameB, bool overwite, bool safe)
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

		// ��¼��һ��lmt�ļ�ԭ���Ĵ�С
		lmtA.seekp(0, ios::end);
		streamoff filesizeA = lmtA.tellp();

		// �ѵڶ���lmt�ļ�����׷�ӵ���һ��lmt�ļ�ĩβ
		jointFiles(lmtA, lmtB);

		// ����ָ��
		if (safe)
		{
			vector<Pointer>pointers;
			scanPointersInLMT(lmtB, pointers);
			updatePointersInLMT(lmtA, pointers, filesizeA);
		}
		else
		{
			lmtB.seekg(0, ios::end);
			streamoff filesizeB = lmtB.tellg();

			// ��С���ܵ�ַ
			streamoff addrMin = 8 + 4 * animationNumB;
			// �����ܵ�ַ
			streamoff addrMax = filesizeB - 1;

			streamoff p = addrMin;
			streamoff v = 0;
			while (p < addrMax)
			{
				lmtB.seekg(p, ios::beg);
				lmtB.read((char *)&v, 4);
				if (v > p && v < addrMax)
				//if (v > addrMin && v < addrMax)
				{
					v += filesizeA;
					lmtA.seekp(p + filesizeA, ios::beg);
					lmtA.write((char *)&v, 4);
				}
				p += 4;
			}
		}

		cout << "Merged animations:" << endl;

		// д����ͷƫ�Ʊ�
		for (int i = 0; i < min(animationNumA, animationNumB); i++)
		{
			if (animationOffsetB[i] && (!animationOffsetA[i] || overwite))
			{
				lmtA.seekp(8 + 4 * i, ios::beg);
				animationOffsetB[i] += filesizeA;
				cout << dec << i << "\t0x" << hex << lmtA.tellp() << "\t0x" << hex << animationOffsetB[i] << endl;
				lmtA.write((char *)&animationOffsetB[i], 4);
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

// �޸Ķ���ͷƫ��
int setAnimationOffset(char *LMTFilename, int animationID, streamoff offset)
{
	int magicNum;
	short version, animationNum;
	uint32_t *animationOffset = nullptr;

	animationOffset = readLMT(LMTFilename, &magicNum, &version, &animationNum);

	if (animationOffset)
	{
		if (animationID >= animationNum)
		{
			cout << "Only " << animationNum << " animations are found in the lmt file: " << LMTFilename << endl;
			return 0;
		} 
		else
		{
			ofstream lmt(LMTFilename, ios::ate | ios::in | ios::binary);
			lmt.seekp(8 + 4 * animationID, ios::beg);
			lmt.write((char *)&offset, 4);
			lmt.close();
		}
	}

	delete[] animationOffset;

	return 1;
}

// ͬһ��lmt�ļ��ڸ��ƶ���
int copyAnimation(char *LMTFilename, int animationID_B, int animationID_A)
{
	int magicNum;
	short version, animationNum;
	uint32_t *animationOffset = nullptr;

	animationOffset = readLMT(LMTFilename, &magicNum, &version, &animationNum);

	if (animationOffset)
	{
		if (animationID_A >= animationNum || animationID_B >= animationNum)
		{
			cout << "Only " << animationNum << " animations are found in the lmt file: " << LMTFilename << endl;
			return 0;
		}
		else
		{
			setAnimationOffset(LMTFilename, animationID_B, animationOffset[animationID_A]);
		}
	}

	delete[] animationOffset;

	return 1;
}

// ������һ��LMT�ļ��ڵĶ���
int copyAnimation(char *LMTFilenameB, char *LMTFilenameA, int animationID_B, int animationID_A, bool safe)
{
	int magicNumA, magicNumB;
	short versionA, animationNumA, versionB, animationNumB;
	uint32_t *animationOffsetA = nullptr;
	uint32_t *animationOffsetB = nullptr;

	animationOffsetA = readLMT(LMTFilenameA, &magicNumA, &versionA, &animationNumA);
	animationOffsetB = readLMT(LMTFilenameB, &magicNumB, &versionB, &animationNumB);

	if (animationID_B >= animationNumB)
	{
		cout << "LMT file: " << animationID_B << " only has " << magicNumB << " animations." << endl;
		return -1;
	}

	if (animationID_A >= animationNumA)
	{
		cout << "LMT file: " << animationID_A << " only has " << magicNumA << " animations." << endl;
		return -1;
	}

	if (animationOffsetA && animationOffsetB)
	{
		ofstream lmtA(LMTFilenameA, ios::ate | ios::in | ios::binary);
		ifstream lmtB(LMTFilenameB, ios::in | ios::binary);

		// ��¼��һ��lmt�ļ�ԭ���Ĵ�С
		lmtA.seekp(0, ios::end);
		streamoff filesizeA = lmtA.tellp();

		// �ѵڶ���lmt�ļ�����׷�ӵ���һ��lmt�ļ�ĩβ
		jointFiles(lmtA, lmtB);

		// ����ָ��
		if (safe)
		{
			vector<Pointer>pointers;
			scanPointersInLMT(lmtB, pointers);
			updatePointersInLMT(lmtA, pointers, filesizeA);
		}
		else
		{
			lmtB.seekg(0, ios::end);
			streamoff filesizeB = lmtB.tellg();

			// ��С���ܵ�ַ
			streamoff addrMin = 8 + 4 * animationNumB;
			// �����ܵ�ַ
			streamoff addrMax = filesizeB - 1;

			streamoff p = addrMin;
			streamoff v = 0;
			while (p < addrMax)
			{
				lmtB.seekg(p, ios::beg);
				lmtB.read((char *)&v, 4);
				if (v > p && v < addrMax)
					//if (v > addrMin && v < addrMax)
				{
					v += filesizeA;
					lmtA.seekp(p + filesizeA, ios::beg);
					lmtA.write((char *)&v, 4);
				}
				p += 4;
			}
		}

		// д����ͷƫ�Ʊ�
		lmtA.seekp(8 + 4 * animationID_A, ios::beg);
		animationOffsetB[animationID_B] += filesizeA;
		cout << dec << animationID_A << "\t0x" << hex << lmtA.tellp() << "\t0x" << hex << animationOffsetB[animationID_B] << endl;
		lmtA.write((char *)&animationOffsetB[animationID_B], 4);

		lmtA.close();
		lmtB.close();

		delete[] animationOffsetA;
		delete[] animationOffsetB;

		return 1;
	}

	if (animationOffsetA)
		delete[] animationOffsetA;
	if (animationOffsetB)
		delete[] animationOffsetB;

	return 0;
}

// ͬһ��lmt�ļ��ڽ�������
int swapAnimation(char *LMTFilename, int animationID_B, int animationID_A)
{
	int magicNum;
	short version, animationNum;
	uint32_t *animationOffset = nullptr;

	animationOffset = readLMT(LMTFilename, &magicNum, &version, &animationNum);

	if (animationOffset)
	{
		if (animationID_A >= animationNum || animationID_B >= animationNum)
		{
			cout << "Only " << animationNum << " animations are found in the lmt file: " << LMTFilename << endl;
			return 0;
		}
		else
		{
			setAnimationOffset(LMTFilename, animationID_A, animationOffset[animationID_B]);
			setAnimationOffset(LMTFilename, animationID_B, animationOffset[animationID_A]);
		}
	}

	delete[] animationOffset;

	return 1;
}

// �������
int clearAnimation(char *LMTFilename, int animationID)
{
	return setAnimationOffset(LMTFilename, animationID, 0);
}

// ����һ��LMT�е��붯��
int importAnimations(char *target, char *source, int animationNum, int animationList[])
{
	int magicNumT, magicNumS;
	short versionT, animationNumT, versionS, animationNumS;
	uint32_t *animationOffsetT = nullptr;
	uint32_t *animationOffsetS = nullptr;

	animationOffsetT = readLMT(target, &magicNumT, &versionT, &animationNumT);
	animationOffsetS = readLMT(source, &magicNumS, &versionS, &animationNumS);
	
	ofstream lmtT(target, ios::ate | ios::in | ios::binary);
	ifstream lmtS(source, ios::in | ios::binary);

	// ��¼Ŀ��lmt�ļ�ԭ���Ĵ�С
	lmtT.seekp(0, ios::end);
	streamoff filesizeT = lmtT.tellp();

	// ��Դlmt�ļ��������ӵ�Ŀ��lmt�ļ�ĩβ
	jointFiles(lmtT, lmtS);

	vector<Pointer>pointers;
	scanPointersInLMT(lmtS, pointers);
	updatePointersInLMT(lmtT, pointers, filesizeT);

	for (int i = 0; i < animationNum; i++)
	{
		if (animationList[i] >= animationNumT)
		{
			cout << "Fail to import animation " << animationList[i] << ":";
			cout << target << " only has " << animationNumT << " animations." << endl;
			continue;
		}
		if (animationList[i] >= animationNumS)
		{
			cout << "Fail to import animation " << animationList[i] << ":";
			cout << source << " only has " << animationNumS << " animations." << endl;
			continue;
		}
		if (animationOffsetS[animationList[i]] == 0)
		{
			cout << "Fail to import animation " << animationList[i] << ":";
			cout << "animation " << animationList[i] << " is null in " << source << " animations." << endl;
			continue;
		}
		lmtT.seekp(8 + 4 * animationList[i], ios::beg);
		animationOffsetS[animationList[i]] += filesizeT;
		lmtT.write((char *)&animationOffsetS[animationList[i]], 4);
		cout << "succeed to import animation " << animationList[i] << endl;
	}

	return 1;
}

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		cout << "Resident Evil Revelations 2 LMT Tool 1.0\nby tiki" << endl;
		/*
		cout << "��ȡ����Σ�������ļ�(.lmt)�ṹ" << endl;
		cout << "Usage: [option] LMTFiles > TXTFile" << endl;
		cout << "option: -r\tread struct info from LMT file" << endl;
		cout << "\t-m\tmerge the animations in the second LMT file into the first one (does not overwrite existing animations)." << endl;
		cout << "\t-mo\tmerge the animations in the second LMT file into the first one (overwrite existing animations)." << endl;
		cout << "\t-ms\tmerge the animations in the second LMT file into the first one (does not overwrite existing animations)(safe mode)." << endl;
		cout << "\t-mos\tmerge the animations in the second LMT file into the first one (overwrite existing animations)(safe mode)." << endl;
		cout << "\t-ca\tcopy animation from B to A by ID in the same LMT file. If only one animation ID is given, remove the animation." << endl;
		cout << "\t-sa\tswap animation A and B by ID in the same LMT file. If only one animation ID is given, remove the animation." << endl;
		cout << "\t-ca LMT_A animeID_A LMT_B animeID_B\tcopy animation from B in LMT file B to A in LMT file A." << endl;
		cout << "\t-cas LMT_A animeID_A LMT_B animeID_B\tcopy animation from B in LMT file B to A in LMT file A (safe mode)." << endl;
		*/
		cout << "Usage: --import targetLMT sourceLMT list_of_animations" << endl;
		cout << "For example:\n\tto import animation 200,201 from pl2200AcA.lmt to pl2400AcA.lmt\n\t--import pl2400AcA.lmt pl2200AcA.lmt 200 201" << endl;
	}
	else
	{
		int magicNum; // ����("LMT")
		short version; // �汾��
		short animationNum; // ������
		uint32_t *animationOffset = nullptr; // ��������ƫ�Ƶ�ַ��
		
		if (strcmp(argv[1], "-r") == 0)	// ��ȡLMT�ļ��ṹ�����
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
			if (strcmp(argv[1], "-m") == 0) // ���ϵڶ���LMT�ļ��еĶ�������һ��LMT�ļ���(�����ǵ�һ��LMT���ִ�Ķ���)
			{
				if (!mergeLMT(argv[2], argv[3], false, false))
					cout << "Fail to merge " << argv[3] << " into " << argv[2] << endl;
			}
			else
			{
				if (strcmp(argv[1], "-mo") == 0) // ���ϵڶ���LMT�ļ��еĶ�������һ��LMT�ļ���(���ǵ�һ��LMT���ִ�Ķ���)
				{
					if (!mergeLMT(argv[2], argv[3], true, false))
						cout << "Fail to merge " << argv[3] << " into " << argv[2] << endl;
				}
				else
				{
					if (strcmp(argv[1], "-ms") == 0) // ���ϵڶ���LMT�ļ��еĶ�������һ��LMT�ļ���(�����ǵ�һ��LMT���ִ�Ķ���)(�޸���������ָ��)
					{
						if (!mergeLMT(argv[2], argv[3], false, true))
							cout << "Fail to merge " << argv[3] << " into " << argv[2] << endl;
					} 
					else
					{
						if (strcmp(argv[1], "-mos") == 0) // ���ϵڶ���LMT�ļ��еĶ�������һ��LMT�ļ���(���ǵ�һ��LMT���ִ�Ķ���)(�޸���������ָ��)
						{
							if (!mergeLMT(argv[2], argv[3], true, true))
								cout << "Fail to merge " << argv[3] << " into " << argv[2] << endl;
						} 
						else
						{
							if (strcmp(argv[1], "-ca") == 0) // ��ͬһ��lmt�ļ��ڸ��ƶ���B��A.���ָֻ����һ������ID��������������.
							{
								/*
								if (argc == 4)
								{
									clearAnimation(argv[2], atoi(argv[3]));
								} 
								else
								{
									if (argc == 5)
									{
										copyAnimation(argv[2], atoi(argv[3]), atoi(argv[4]));
									}
									else
									{
										cout << "Parameters are less or more than required." << endl;
									}
								}
								*/
								switch (argc)
								{
								case 4:
									clearAnimation(argv[2], atoi(argv[3]));
									break;
								case 5:
									copyAnimation(argv[2], atoi(argv[3]), atoi(argv[4]));
									break;
								case 6:
									copyAnimation(argv[4], argv[2], atoi(argv[5]), atoi(argv[3]), false);
									break;
								default:
									cout << "Parameters are less or more than required." << endl;
								}
							} 
							else
							{
								if (strcmp(argv[1], "-sa") == 0) // ��ͬһ��lmt�ļ��ڽ���2������.���ָֻ����һ������ID��������������.
								{
									if (argc == 4)
									{
										clearAnimation(argv[2], atoi(argv[3]));
									}
									else
									{
										if (argc == 5)
										{
											swapAnimation(argv[2], atoi(argv[3]), atoi(argv[4]));
										}
										else
										{
											cout << "Parameters are less or more than required." << endl;
										}
									}
								} 
								else
								{
									if (strcmp(argv[1], "-cas") == 0)
									{
										switch (argc)
										{
										case 4:
											clearAnimation(argv[2], atoi(argv[3]));
											break;
										case 5:
											copyAnimation(argv[2], atoi(argv[3]), atoi(argv[4]));
											break;
										case 6:
											copyAnimation(argv[4], argv[2], atoi(argv[5]), atoi(argv[3]), true);
											break;
										default:
											cout << "Parameters are less or more than required." << endl;
										}
									} 
									else
									{
										if (strcmp(argv[1], "--import") == 0)
										{
											int totalNum = argc - 4;
											int *animationID = new int[totalNum];
											for (int i = 0; i < totalNum; i++)
											{
												animationID[i] = atoi(argv[i + 4]);
											}
											importAnimations(argv[2], argv[3], totalNum, animationID);
											delete[] animationID;
										} 
										else
										{
											cout << "Unsupported option: " << argv[1] << endl;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

