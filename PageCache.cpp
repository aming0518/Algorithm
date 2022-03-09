#include "PageCache.h"

//����PageCacheҲ��Ҫ�����ģ�����������Ȼ�Ҫ��Ҫ������PageCache����ס����Ϊ��CentralCache����Span��ʱ���п��ܶ�Ӧ��λ��û����ô���ҳ
//����������Ҫȥ����ң����Ƿ��и����ҳ��Ȼ����������и�����п��ܻ��������PageCache

PageCache PageCache::_sInst;
// ��ϵͳ����kҳ�ڴ�

void* PageCache::SystemAllocPage(size_t k)
{
	return ::SystemAlloc(k); 
}



//Ҫ4ҳΪʲô��ֱ����ϵͳ����һ��4ҳ���ڴ棬��������һ��128ҳ���ڴ棬Ȼ���и��أ��������Ǹ����鷳��
//�ô�����---��Ϊ���ǿ����ںϳ�һ����ڴ棬�����ͱ���������Ҫ���ڴ��ʱ�������ڴ���Ƭ�����⵼���޷�����
Span* PageCache::NewSpan(size_t k)
{
	//recursive_mutexר�Ÿ��ݹ�ʹ�õ���
	std::lock_guard<std::recursive_mutex> lock(_mtx); //��Ϊ������һ���ݹ���ã�����ԭ�ȵ�����Դ���������ڻ�û�е����ͻᱨbusy�Ĵ�������Ҫ����
	//���ֱ���������NPAGES�Ĵ���ڴ棬ֱ����ϵͳҪ
	if (k >= NPAGES)
	{
		void* ptr = SystemAllocPage(k);
		//��Ȼ����ϵͳҪ���������128ҳ�Ĵ���ڴ棬����ʹ�õ�ͬ�����߼������Ի���Ҫ����һ��Span��
		Span* span = new Span;
		span->_pageId = (ADDRES_INT)ptr >> PAGE_SHIFT;
		span->_n = k;

		_idSpanMap[span->_pageId] = span;
		return span;
	}
	if (!_spanList[k].Empty())
	{
		/*Span* it = _spanList[k].Begin();
		_spanList[k].Erase(it);
		return it;*/

		return _spanList[k].PopFront();
	}
	//splitspan��ʣ���
	for (size_t i = k + 1; i < NPAGES; ++i)
	{
		// ��ҳ����С,�г�kҳ��span����
		// �г�i-kҳ�һ���������
		if (!_spanList[i].Empty())
		{
			//������ͷ�У������ã���Ϊ�Ͼ������ڴ����ʶ��������Сҳ����ô����ĺܴ�һ����ҳ�Ŷ�Ҫ����ӳ��
			//�����Ϊβ�У���ôֻ��Ҫ�Ķ���Сһ���ֵ�ӳ���ϵ�Ϳ�����
			//Span* span = _spanList[i].Begin();
			//_spanList[i].Erase(span);       //���λ�øо�����      ����
   //                                                   //100   |
			//Span* splitSpan = new Span;               //------------------------------------
			//splitSpan->_pageId = span->_pageId + k;   //------------------------------------
			//splitSpan->_n = span->_n - k;

			//span->_n = k;

			//_spanList[splitSpan->_n].Insert(_spanList[splitSpan->_n].Begin(), splitSpan);

			//return span;

			// β�г�һ��kҳspan
			Span* span = _spanList[i].PopFront();

			Span* split = new Span;
			split->_pageId = span->_pageId + span->_n - k;
			split->_n = k;

			// �ı��г���span��ҳ�ź�span��ӳ���ϵ
			for (PageID i = 0; i < k; ++i)
			{
				_idSpanMap[split->_pageId + i] = split;
			}

			//����span��˵ֻ���޸�һ��ҳ����������Ҫ����Ӧ�Ĺ�ϵ
			span->_n -= k;

			_spanList[span->_n].PushFront(span);

			return split;
		}
	}

	Span* bigSpan = new Span;
	void* memory = SystemAllocPage(NPAGES - 1);
	bigSpan->_pageId = (size_t)memory >> 12; //ָ����Ƕ��ֽڵı��
	bigSpan->_n = NPAGES - 1;
	// ��ҳ�ź�spanӳ���ϵ����
	for (PageID i = 0; i < bigSpan->_n; ++i)
	{
		PageID id = bigSpan->_pageId + i;
		_idSpanMap[id] = bigSpan;
	}
	_spanList[NPAGES - 1].Insert(_spanList[NPAGES - 1].Begin(), bigSpan);

	return NewSpan(k);
}



Span* PageCache::MapObjectToSpan(void* obj)
{
	PageID id = (ADDRES_INT)obj >> PAGE_SHIFT;
//	auto ret = _idSpanMap.find(id);
//	if (ret != _idSpanMap.end())
//	{
//		return ret->second;
//	}
//	else
//	{
//		//Ӧ��һ�����ܹ��ҵ��ģ���Ϊÿһ��С�ڴ涼������Span
//		//����Ҳ����ͳ��ִ������ˣ���������ѡ�����ֱִ��ķ�ʽֱ�Ӷ���
//		assert(false);  //������������
//		return  nullptr; //����Ӧ��һ�����᷵�ؿ�
//	}

	Span* span = _idSpanMap.get(id);
	if (span != nullptr)
	{
		return span;
	}
	else
	{
		assert(false);
		return nullptr;
	}
}

void PageCache::ReleaseSpanToPageCache(Span* span)
{
	//�������128ҳ�Ĵ���ڴ���ͷ�����
	if (span->_n >= NPAGES)
	{
		//_idSpanMap.erase(span->_pageId);
		_idSpanMap.erase(span->_pageId);
		void* ptr = (void*)(span->_pageId << PAGE_SHIFT);
		SystemFree(ptr);
		delete span;
		return;
	}

	std::lock_guard<std::recursive_mutex> lock(_mtx);

	// ���ǰ�����spanҳ�����кϲ�,����ڴ���Ƭ����

	//�ϲ��Ĺ���Ӧ����һֻ������ȥ�ģ�ֱ��ǰһ��Span���ǿ��е�
	// ��ǰ�ϲ�
	while (1)
	{
		PageID preId = span->_pageId - 1;
		//auto ret = _idSpanMap.find(preId);
		//// ���ǰһ��ҳ��span�����ڣ�ϵͳδ���䣬������ǰ�ϲ�
		//if (ret == _idSpanMap.end())
		//{
		//	break;
		//}
		Span* preSpan = _idSpanMap.get(preId);
		if (preSpan == nullptr)
		{
			break;
		}

		// ���ǰһ��ҳ��span����ʹ���У�������ǰ�ϲ�
		if (preSpan->_usecount != 0)
		{
			break;
		}

		// ��ʼ�ϲ�... ��ʱ���Ѿ�����128ҳ�ˣ����Բ�Ҫ�ڽ��кϲ��ˣ�û�������ˣ�û�ط�����
		if (preSpan->_n + span->_n >= NPAGES)
		{
			break;
		}

		//Span������Ǵ�CentralCache�з������ģ���û�в��뵽PageCache��
		// �Ӷ�Ӧ��span�����н��������ٺϲ�
		_spanList[preSpan->_n].Erase(preSpan);

		span->_pageId = preSpan->_pageId;
		span->_n += preSpan->_n;

		// ����ҳ֮��ӳ���ϵ������Ҫȫ�������и��ģ�ֻ��Ҫ��preSpan�еĽ��иı�Ϳ�����
		for (PageID i = 0; i < preSpan->_n; ++i)
		{
			_idSpanMap[preSpan->_pageId + i] = span;
		}

		delete preSpan;
	}

	// ���ϲ�
	while (1)
	{
		PageID nextId = span->_pageId + span->_n; //����벻ͨ�������Ӵ�����з��� PageID 100  һ��2ҳ ��ô��һҳ����102
		//auto ret = _idSpanMap.find(nextId);
		//if (ret == _idSpanMap.end())
		//{
		//	break;
		//}
		Span* nextSpan = _idSpanMap.get(nextId);
		if (nextSpan == nullptr)
		{
			break;
		}

		if (nextSpan->_usecount != 0)
		{
			break;
		}

		//��������߼�һ����������Ѿ���128ҳ�ˣ��Ͳ�Ҫ�ټ����������ȥ�ϲ���
		if (nextSpan->_n + span->_n >= NPAGES)
		{
			break;
		}

		_spanList[nextSpan->_n].Erase(nextSpan);

		span->_n += nextSpan->_n;
		for (PageID i = 0; i < nextSpan->_n; ++i)
		{
			_idSpanMap[nextSpan->_pageId + i] = span;
		}

		delete nextSpan;
	}

	// �ϲ����Ĵ�span�����뵽��Ӧ��������
	_spanList[span->_n].PushFront(span);
}