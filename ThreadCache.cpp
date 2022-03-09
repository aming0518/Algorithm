#include "ThreadCache.h"
#include "CentralCache.h"

void* ThreadCache::FetchFromCentralCache(size_t i, size_t size)
{
	// ��ȡһ����������ʹ����������ʽ
	//��Ϊ�����һ���Ը���̫�࣬�����Ժ��ڲ�ʹ���ˣ��ͻ�����˷ѣ�������õİ취���ǲ�������������ʼ�٣�����������Ҫ�Ĵ���Խ��Խ�࣬
	//��ô��ÿ�ζ����һЩ���ҽ������freelist����
	size_t batchNum = min(SizeClass::NumMoveSize(size), _freeLists[i].MaxSize());

	// ȥ���Ļ����ȡbatch_num������
	void* start = nullptr;
	void* end = nullptr;
	//Ҫ��һ�����ص�һ������ĵ�ַ��ʣ�µĶ��ҽ�����������Ҳ�п��������Span���治�����batch_num����ô�ٸ��㷵�ؼ�����ʵҲ�ǿ���
	//��Ϊ�Ѿ��������������Ҫ��Ҫ��
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, SizeClass::RoundUp(size));
	assert(actualNum > 0);

	// >1������һ����ʣ�¹ҵ���������
	//���һ����������ʣ�µĹ��������´�����Ͳ���Ҫ�����Ļ���
	//�������ľ���
	if (actualNum > 1)
	{
		_freeLists[i].PushRange(NextObj(start), end, actualNum - 1);
	}

	if (_freeLists[i].MaxSize() == batchNum)
	{
		_freeLists[i].SetMaxSize(_freeLists[i].MaxSize() + 1);
	}

	return start;
}

void* ThreadCache::Allocate(size_t size)
{
	size_t i = SizeClass::Index(size);
	if (!_freeLists[i].Empty())
	{
		return _freeLists[i].Pop();
	}
	else
	{
		//�������������ʵ�������ֿ��ܵ�һ������CentralCache������һ����CentralCacheҲû�У��Ǿ�ֻ����PageCacheҪ
		return FetchFromCentralCache(i, size);
	}
}

void ThreadCache::Deallocate(void* ptr, size_t size)
{
	size_t i = SizeClass::Index(size);
	_freeLists[i].Push(ptr);

	// List Too Long central cache ȥ�ͷ�
	//����ʹ�õ��ǳ�����ȷ���Ƿ���Ҫ���й黹��Ϊ����
	if (_freeLists[i].Size() > _freeLists[i].MaxSize())
	{
		ListTooLong(_freeLists[i], size);
	}
}

// �ͷŶ���ʱ���������ʱ�������ڴ�ص����Ļ���
void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	size_t batchNum = list.MaxSize(); //Ҫ�����ٸ�
	void* start = nullptr;
	void* end = nullptr;
	list.PopRange(start, end, batchNum);

	CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}