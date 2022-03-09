#pragma once
#include "Common.h"

//CentralCache��Ҫ�����ģ������������Ȳ���Ҫ̫��ֻ��Ҫ��һ��Ͱ������Ϊֻ�ж���߳�ͬʱȡһ��Span
//Ҫ��֤CentralCache��PageCache������ȫ��Ψһ�ģ�����ֱ��ʹ�õ���ģʽ
//������ʹ�õ��Ƕ���ģʽ---һ��ʼ�ͽ��д�����main����֮ǰ�ͽ����˴�����
class CentralCache
{
public:
	//����ָ��������ö��ǿ��Ե�
	static CentralCache* GetInstance()
	{
		return &_inst;
	}


	// �����Ļ����ȡһ�������Ķ����thread cache
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t byte_size);

	// ��SpanList����page cache��ȡһ��span
	Span* GetOneSpan(SpanList& list, size_t byte_size);


	// ��һ�������Ķ����ͷŵ�span���
	void ReleaseListToSpans(void* start, size_t byte_size);
private:
	SpanList _spanLists[NFREELISTS]; // �����뷽ʽӳ��    �����span���й��ˣ�������һ����С�����Ѿ��зֳ�ȥ��

private:

	CentralCache() = default;
	CentralCache(const CentralCache&) = delete;
	CentralCache& operator=(const CentralCache&) = delete;

	static CentralCache _inst;
};
