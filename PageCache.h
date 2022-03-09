#pragma once
#include "Common.h"
#include "PageMap.h"

class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_sInst;
	}
	// ��ϵͳ����kҳ�ڴ�ҵ���������
	void* SystemAllocPage(size_t k);

	Span* NewSpan(size_t k);

	//��ȡ�Ӷ���span��ӳ��
	Span* MapObjectToSpan(void* obj);

	void ReleaseSpanToPageCache(Span* span);
private:
	SpanList _spanList[NPAGES];	// ��ҳ��ӳ��

	//std::map<PageID, Span*> _idSpanMap; //�������п��ܶ��ҳ��ӳ��ͬһ��Span��
	//tcmalloc ������ Ч�ʸ���

	TCMalloc_PageMap2<32-PAGE_SHIFT> _idSpanMap;



	std::recursive_mutex _mtx;
private:
	PageCache() = default;
	PageCache(const PageCache&) = delete;
	PageCache& operator=(const PageCache&) = delete;

	static PageCache _sInst;
};




