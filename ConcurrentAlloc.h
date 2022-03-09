#pragma once

//��Ҫ�������ڸ߲���������º�malloc��free���жԱ�����
#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"

//void* tcmalloc(size_t size)
static void* ConcurrentAlloc(size_t size)
{
	try
	{
		if (size > MAX_BYTES)
		{
			// ��PageCacheҪ������Ҳ��Ҫ�����ģ���������������ô��Ŀռ�ĳ����Ͼ��������ģ����Բ���Ӱ��Ч��
			//�����Ҫ65KB����ô�͸���68KB����Ϊ��PageCache�У�������ҳΪ��λ��
			size_t npage = SizeClass::RoundUp(size) >> PAGE_SHIFT;
			//��ʹ�Ǵ���128ҳ��Ҳʹ�õ���ͬ�����߼������Զ���NewSpan������Ҫ��������ô����NewSpan()��Ȼ�Դ���128ҳ�����˵���,���ͷŵ�ʱ��
			//����ReleaseSpanToPageCache()��ҲӦ����������
			Span* span = PageCache::GetInstance()->NewSpan(npage);
			span->_objsize = size; //��17ҳ��129ҳ����������

			void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
			return ptr;
		}
		else
		{
			if (tls_threadcache == nullptr)
			{
				tls_threadcache = new ThreadCache;
			}

			return tls_threadcache->Allocate(size);
		}
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
	return nullptr;
}

//�ͷŵ�ʱ��Ӧ���к���Ĵ�С����Ϊ������free����û�е�
static void ConcurrentFree(void* ptr)
{
	try
	{
		Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
		size_t size = span->_objsize;

		if (size > MAX_BYTES)
		{
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		}
		else
		{
			assert(tls_threadcache);
			tls_threadcache->Deallocate(ptr, size);
		}
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl;
	}
}