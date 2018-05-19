// Resident Evil LMT Tool.cpp : 定义控制台应用程序的入口点。
// 生化危机动作文件.lmt的mod辅助工具
// by 拉切尔
// 2018-05-09

#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// #include "IniFile.h"
// 只有读取ini和取值,没有写入ini和赋值,故更换如下
#include "CMyINI.h"

using namespace std;

// 取两个数的最小值
#define min(x,y)	((x)<(y)?(x):(y))

// 读取LMT文件结构
uint32_t *readLMT(char *LMTFilename, unsigned int *magicNum, unsigned short *version, unsigned short *animationNum)
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

// 联结文件
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

// 指针
struct Pointer
{
	streamoff self;	// 自己的偏移地址
	streamoff target; // 指向的偏移地址
};

// 扫描LMT内指针
int scanPointersInLMT(ifstream &lmt, vector<Pointer>&pointers)
{
	// 读取动画总数
	short animationNum;
	lmt.seekg(6, ios::beg);
	lmt.read((char *)&animationNum, 2);

	// 读取动画头偏移表
	uint32_t *animationOffset = new uint32_t[animationNum];
	lmt.seekg(8, ios::beg);
	lmt.read((char *)animationOffset, animationNum * 4);
	
	// 动画头0x40字节，含有3个指针
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

			// 动画头中的第一个指针指向关键帧定义结构体数组，该结构体长度为0x24，含有2个指针
			if (point.target)
			{
				for (int j = 0; j < boneCount; j++)
				{
					// bufferSize不是指针
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

			// 第2个指针指向的结构体数组,结构体长度0x48,最后4个字节是指针,数组长度4
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

			// 第3个指针指向的结构体数组,结构体长度12,最后4个字节是指针,数组长度4
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

// 改写LMT内指针
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

// 合并LMT(A<-B)
int mergeLMT(char *LMTFilenameA, char *LMTFilenameB, bool overwite, bool safe)
{
	unsigned int magicNumA, magicNumB;
	unsigned short versionA, animationNumA, versionB, animationNumB;
	uint32_t *animationOffsetA = nullptr;
	uint32_t *animationOffsetB = nullptr;

	animationOffsetA = readLMT(LMTFilenameA, &magicNumA, &versionA, &animationNumA);
	animationOffsetB = readLMT(LMTFilenameB, &magicNumB, &versionB, &animationNumB);

	if (animationOffsetA && animationOffsetB)
	{
		ofstream lmtA(LMTFilenameA, ios::ate | ios::in | ios::binary);
		ifstream lmtB(LMTFilenameB, ios::in | ios::binary);

		// 记录第一个lmt文件原来的大小
		lmtA.seekp(0, ios::end);
		streamoff filesizeA = lmtA.tellp();

		// 把第二个lmt文件整个追加到第一个lmt文件末尾
		jointFiles(lmtA, lmtB);

		// 处理指针
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

			// 最小可能地址
			streamoff addrMin = 8 + 4 * animationNumB;
			// 最大可能地址
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

		// 写动画头偏移表
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

// 修改动画头偏移
int setAnimationOffset(char *LMTFilename, int animationID, streamoff offset)
{
	unsigned int magicNum;
	unsigned short version, animationNum;
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

// 同一个lmt文件内复制动画
int copyAnimation(char *LMTFilename, int animationID_B, int animationID_A)
{
	unsigned int magicNum;
	unsigned short version, animationNum;
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

// 复制另一个LMT文件内的动画
int copyAnimation(char *LMTFilenameB, char *LMTFilenameA, int animationID_B, int animationID_A, bool safe)
{
	unsigned int magicNumA, magicNumB;
	unsigned short versionA, animationNumA, versionB, animationNumB;
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

		// 记录第一个lmt文件原来的大小
		lmtA.seekp(0, ios::end);
		streamoff filesizeA = lmtA.tellp();

		// 把第二个lmt文件整个追加到第一个lmt文件末尾
		jointFiles(lmtA, lmtB);

		// 处理指针
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

			// 最小可能地址
			streamoff addrMin = 8 + 4 * animationNumB;
			// 最大可能地址
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

		// 写动画头偏移表
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

// 同一个lmt文件内交换动画
int swapAnimation(char *LMTFilename, int animationID_B, int animationID_A)
{
	unsigned int magicNum;
	unsigned short version, animationNum;
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

// 清除动画
int clearAnimation(char *LMTFilename, int animationID)
{
	return setAnimationOffset(LMTFilename, animationID, 0);
}

// 从另一个LMT中导入动画
int importAnimations(char *target, char *source, int animationNum, int animationList[])
{
	unsigned int magicNumT, magicNumS;
	unsigned short versionT, animationNumT, versionS, animationNumS;
	uint32_t *animationOffsetT = nullptr;
	uint32_t *animationOffsetS = nullptr;

	animationOffsetT = readLMT(target, &magicNumT, &versionT, &animationNumT);
	animationOffsetS = readLMT(source, &magicNumS, &versionS, &animationNumS);
	
	ofstream lmtT(target, ios::ate | ios::in | ios::binary);
	ifstream lmtS(source, ios::in | ios::binary);

	// 记录目标lmt文件原来的大小
	lmtT.seekp(0, ios::end);
	streamoff filesizeT = lmtT.tellp();

	// 把源lmt文件整个连接到目标lmt文件末尾
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

// 输出十六进制串,逗号分隔
string to_sequence(int num, char seq[])
{
	stringstream ss;
	for (int i = 0; i < num - 1; i++)
	{
		ss << hex << ((unsigned)seq[i] & 0xff) << ",";
	}
	ss << hex << ((unsigned)seq[num - 1] & 0xff);
	return ss.str();
}

// 输出浮点数串,逗号分隔
string to_sequence(int num, float seq[])
{
	stringstream ss;
	for (int i = 0; i < num - 1; i++)
	{
		ss << seq[i] << ",";
	}
	ss << seq[num - 1];
	return ss.str();
}

// 输出偏移串,逗号分隔
string to_sequence(int num, streamoff seq[])
{
	stringstream ss;
	for (int i = 0; i < num - 1; i++)
	{
		ss << hex << seq[i] << ",";
	}
	ss << hex << seq[num - 1];
	return ss.str();
}

// 解包列表里指定的动画
int unpackAnimationsInList(char *outpath, char *lmtName, int anmationListLength, int *anmationIDList)
{
	unsigned int magicNum;
	unsigned short version, animationNum;
	uint32_t *animationOffset = nullptr;

	animationOffset = readLMT(lmtName, &magicNum, &version, &animationNum);

	if (animationOffset)
	{
		ifstream lmt(lmtName, ios::in | ios::binary);

		for (int i = 0; i < anmationListLength; i++)
		{
			int anmationID = anmationIDList[i];
			// cout << i << "\t" << anmationID << endl;
			if (anmationID >= animationNum)
			{
				cout << "Fail to unpack animation " << anmationID << ": animation does not exist." << endl;
				continue;
			} 
			else
			{
				if (animationOffset[anmationID] == 0)
				{
					cout << "Fail to unpack animation " << anmationID << ": animation is NULL." << endl;
					continue;
				} 
				else
				{
					stringstream ss;

					// 动画头
					/*lmt.seekg(animationOffset[anmationID] + 4, ios::beg);
					char *buffer = new char[48];
					lmt.read(buffer, 48);
					ss << anmationID << ".ah";
					ofstream animationHeader(ss.str().c_str(), ios::binary);
					animationHeader.write(buffer, 48);
					animationHeader.close();
					delete[] buffer;*/

					// 第一个指针
					/*streamoff ptr0 = 0;
					lmt.seekg(animationOffset[anmationID], ios::beg);
					lmt.read((char *)&ptr0, 4);
					if (ptr0)
					{
						int boneCount = 0;
						lmt.seekg(animationOffset[anmationID] + 4, ios::beg);
						lmt.read((char *)&boneCount, 4);
						lmt.seekg(ptr0, ios::beg);
					}*/

					ss << outpath << "\\" << anmationID << ".ini";
					// ofstream animationHeader(ss.str().c_str());
					
					CMyINI *ini = new CMyINI();

					lmt.seekg(animationOffset[anmationID], ios::beg);

					streamoff ptr1, ptr2, ptr3;
					ptr1 = ptr2 = ptr3 = 0;
					lmt.read((char *)&ptr1, 4);

					unsigned int boneCount = 0;
					// lmt.seekg(animationOffset[anmationID] + 4, ios::beg);
					lmt.read((char *)&boneCount, 4);
					ini->SetValue("setting", "boneCount", to_string(boneCount));

					unsigned int FrameCount = 0;
					// lmt.seekg(animationOffset[anmationID] + 8, ios::beg);
					lmt.read((char *)&FrameCount, 4);
					ini->SetValue("setting", "FrameCount", to_string(FrameCount));

					int LoopFrame = -1;
					// lmt.seekg(animationOffset[anmationID] + 12, ios::beg);
					lmt.read((char *)&LoopFrame, 4);
					ini->SetValue("setting", "LoopFrame", to_string(LoopFrame));

					char others[36];
					// lmt.seekg(animationOffset[anmationID] + 16, ios::beg);
					lmt.read(others, 36);
					ini->SetValue("setting", "others(hexadecimal)", to_sequence(36, others));

					lmt.read((char *)&ptr2, 4);
					lmt.read((char *)&ptr3, 4);

					// cout << hex << ptr1 << "\t" << ptr2 << "\t" << ptr3 << endl;

					if (ptr1)
					{
						for (int j = 0; j < boneCount; j++)
						{
							stringstream bone;
							bone << "bone" << j;

							lmt.seekg(ptr1 + 0x24 * j, ios::beg);

							uint8_t bufferType, usage, jointType, boneID;
							lmt.read((char *)&bufferType, 1);
							lmt.read((char *)&usage, 1);
							lmt.read((char *)&jointType, 1);
							lmt.read((char *)&boneID, 1);
							ini->SetValue(bone.str(), "bufferType", to_string(bufferType));
							ini->SetValue(bone.str(), "usage", to_string(usage));
							ini->SetValue(bone.str(), "jointType", to_string(jointType));
							ini->SetValue(bone.str(), "boneID", to_string(boneID));

							float weight = 0;
							lmt.read((char *)&weight, 4);
							ini->SetValue(bone.str(), "weight", to_string(weight));

							int bufferSize = 0;
							lmt.read((char *)&bufferSize, 4);
							ini->SetValue(bone.str(), "bufferSize", to_string(bufferSize));

							streamoff buffer = 0;
							lmt.read((char *)&buffer, 4);

							float referenceFrame[4];
							lmt.read((char *)referenceFrame, 4 * 4);
							/*ini->SetValue(bone.str(), "referenceFrame0", to_string(referenceFrame[0]));
							ini->SetValue(bone.str(), "referenceFrame1", to_string(referenceFrame[1]));
							ini->SetValue(bone.str(), "referenceFrame2", to_string(referenceFrame[2]));
							ini->SetValue(bone.str(), "referenceFrame3", to_string(referenceFrame[3]));*/
							ini->SetValue(bone.str(), "referenceFrame", to_sequence(4, referenceFrame));

							if (version > 55)
							{
								streamoff bounds = 0;
								lmt.read((char *)&bounds, 4);

								if (bounds)
								{
									lmt.seekg(bounds, ios::beg);
									float addin[4], offset[4];
									lmt.read((char *)addin, 4 * 4);
									lmt.read((char *)offset, 4 * 4);
									ini->SetValue(bone.str(), "addin", to_sequence(4, addin));
									ini->SetValue(bone.str(), "offset", to_sequence(4, offset));
								}
							}

							if (bufferSize > 0 && buffer)
							{
								lmt.seekg(buffer, ios::beg);
								// 直接生dump
								char *bufferRaw = new char[bufferSize];
								lmt.read(bufferRaw, bufferSize);
								ini->SetValue(bone.str(), "buffer(raw)", to_sequence(bufferSize, bufferRaw));
								delete[] bufferRaw;

								//TODO 根据不同的bufferType解析buffer中的数据

							} 
							else
							{
								//TODO 建LMTUniKey?

							}
						}
					}

					if (ptr2)
					{
						// cout << hex << ptr2 << endl;
						lmt.seekg(ptr2, ios::beg);
						streamoff partLastPtr[4];
						for (int j = 0; j < 4; j++)
						{
							char *part = new char[0x44];
							lmt.read(part, 0x44);
							stringstream partName;
							partName << "part" << j;
							ini->SetValue("pointer2", partName.str(), to_sequence(0x44, part));
							delete[] part;
							partLastPtr[j] = 0;
							lmt.read((char *)&partLastPtr[j], 4);
							if (partLastPtr[j])
							{
								// partLastPtr[j] -= ptr2;
								partLastPtr[j] -= (ptr2 + 0x48 * 4);
							}
						}
						ini->SetValue("pointer2", "pointers", to_sequence(4, partLastPtr));
						// streamoff commonEnd = 0;
						// 通过下一个指针判断这个指针指向数据的结尾
						/*if (ptr3)
						{
							commonEnd = ptr3;
						} 
						else
						{
							streampos savePos = lmt.tellg();
							if (anmationID == animationNum - 1) // 最后一个动画
							{
								lmt.seekg(0, ios::end);
								commonEnd = lmt.tellg();
							} 
							else
							{
								int j = 1;
								while (0 == commonEnd)
								{
									lmt.seekg(animationOffset[anmationID + j++], ios::beg);
									lmt.read((char *)&commonEnd, 4);
									if (0 == commonEnd)
									{
										lmt.seekg(0x30, ios::cur);
										lmt.read((char *)&commonEnd, 4);
									}
									if (0 == commonEnd)
									{
										lmt.read((char *)&commonEnd, 4);
									}
									if (anmationID + j >= animationNum)
									{
										break;
									}
								}
							}
							lmt.seekg(savePos, ios::beg);
						}*/
						// 改进:最后一个指针指向最后8个字节,以此判断数据结尾
						// commonEnd = partLastPtr[3] + 8;
						int commonLength = partLastPtr[3] + 8;
						// if (commonEnd)
						if(commonLength > 0)
						{
							// int commonLength = commonEnd - ptr2 - 0x48 * 4;
							// cout << commonLength << endl;
							char *common = new char[commonLength];
							lmt.read(common, commonLength);
							ini->SetValue("pointer2", "common", to_sequence(commonLength, common));
						}
					}

					if (ptr3)
					{
						lmt.seekg(ptr3, ios::beg);
						streamoff partLastPtr[4];
						for (int j = 0; j < 4; j++)
						{
							char *part = new char[8];
							lmt.read(part, 8);
							stringstream partName;
							partName << "part" << j;
							ini->SetValue("pointer3", partName.str(), to_sequence(8, part));
							delete[] part;
							partLastPtr[j] = 0;
							lmt.read((char *)&partLastPtr[j], 4);
							if (partLastPtr[j])
							{
								// partLastPtr[j] -= ptr3;
								partLastPtr[j] -= (ptr3 + 12 * 4);
							}
						}
						ini->SetValue("pointer3", "pointers", to_sequence(4, partLastPtr));
						// streamoff commonEnd = 0;
						// 最后一个指针指向数据结尾
						// commonEnd = partLastPtr[3];
						int commonLength = partLastPtr[3];
						// if (commonEnd)
						if(commonLength > 0)
						{
							// int commonLength = commonEnd - ptr3 - 12 * 4;
							char *common = new char[commonLength];
							lmt.read(common, commonLength);
							ini->SetValue("pointer3", "common", to_sequence(commonLength, common));
						}
					}

					ini->WriteINI(ss.str());
				}
			}
		}

		lmt.close();

		return 1;
	}
	else
	{
		cout << "Fail to load .lmt file: " << lmtName << endl;
		return 0;
	}
}

// 解包所有的动画
int unpackAllAnimations(char *outpath, char *lmtName)
{
	unsigned int magicNum;
	unsigned short version, animationNum;
	uint32_t *animationOffset = nullptr;

	animationOffset = readLMT(lmtName, &magicNum, &version, &animationNum);

	if (animationOffset)
	{
		int realAnimationNum = 0;
		for (int i = 0; i < animationNum; i++)
		{
			if (animationOffset[i])
			{
				realAnimationNum++;
			}
		}
		int *realAnimationList = new int[realAnimationNum];
		if (realAnimationList)
		{
			for (int i = 0, j = 0; i < animationNum || j < realAnimationNum; i++)
			{
				if (animationOffset[i])
				{
					// cout << i << "\t" << j << endl;
					realAnimationList[j++] = i;
				}
			}
			// cout << realAnimationNum << "\t" << realAnimationList << endl;
			if (realAnimationNum)
			{
				unpackAnimationsInList(outpath, lmtName, realAnimationNum, realAnimationList);
			}
			delete[] realAnimationList;
		}
		return 1;
	}
	else
	{
		cout << "Fail to load .lmt file: " << lmtName << endl;
		return 0;
	}
}

int main(int argc, char *argv[])
{
	if (argc == 1)
	{
		cout << "Resident Evil Revelations 2 LMT Tool 1.0\nby tiki" << endl;
		/*
		cout << "读取生化危机动作文件(.lmt)结构" << endl;
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
		cout << "Usage:\n--import targetLMT sourceLMT list_of_animations" << endl;
		cout << "\tFor example:\n\tto import animation 200,201 from pl2200AcA.lmt to pl2400AcA.lmt\n\t--import pl2400AcA.lmt pl2200AcA.lmt 200 201" << endl;
		cout << "--unpack OutputPath LMTFile [AnimationID]" << endl;
		cout << "\tIf animation ID is not set, unpack all animations in the .lmt file." << endl;
		cout << "--repack InputPath [LMTFile]" << endl;
		cout << "\tIf the output .lmt file is neither given nor found , create a new one." << endl;
	}
	else
	{
		unsigned int magicNum; // 幻数("LMT")
		unsigned short version; // 版本号
		unsigned short animationNum; // 动画数
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
				if (!mergeLMT(argv[2], argv[3], false, false))
					cout << "Fail to merge " << argv[3] << " into " << argv[2] << endl;
			}
			else
			{
				if (strcmp(argv[1], "-mo") == 0) // 整合第二个LMT文件中的动画到第一个LMT文件里(覆盖第一个LMT里现存的动画)
				{
					if (!mergeLMT(argv[2], argv[3], true, false))
						cout << "Fail to merge " << argv[3] << " into " << argv[2] << endl;
				}
				else
				{
					if (strcmp(argv[1], "-ms") == 0) // 整合第二个LMT文件中的动画到第一个LMT文件里(不覆盖第一个LMT里现存的动画)(修改所有疑似指针)
					{
						if (!mergeLMT(argv[2], argv[3], false, true))
							cout << "Fail to merge " << argv[3] << " into " << argv[2] << endl;
					} 
					else
					{
						if (strcmp(argv[1], "-mos") == 0) // 整合第二个LMT文件中的动画到第一个LMT文件里(覆盖第一个LMT里现存的动画)(修改所有疑似指针)
						{
							if (!mergeLMT(argv[2], argv[3], true, true))
								cout << "Fail to merge " << argv[3] << " into " << argv[2] << endl;
						} 
						else
						{
							if (strcmp(argv[1], "-ca") == 0) // 在同一个lmt文件内复制动画B到A.如果只指定了一个动画ID，则清除这个动画.
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
								if (strcmp(argv[1], "-sa") == 0) // 在同一个lmt文件内交换2个动画.如果只指定了一个动画ID，则清除这个动画.
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
											if (strcmp(argv[1], "--unpack") == 0)
											{
												if (argc < 4)
												{
													cout << "Parameters are less than required." << endl;
												} 
												else
												{
													if (argc > 4)
													{
														int *animationIDList = new int[argc - 4];
														for (int i = 0; i < argc - 4; i++)
														{
															animationIDList[i] = atoi(argv[4 + i]);
															// cout << animationIDList[i] << endl;
														}
														unpackAnimationsInList(argv[2], argv[3], argc - 4, animationIDList);
														delete[] animationIDList;
													} 
													else
													{
														unpackAllAnimations(argv[2], argv[3]);
													}
												}
											} 
											else
											{
												if (strcmp(argv[1], "--repack") == 0)
												{
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
		}
	}
	return 0;
}

