#include "CentralCache.h"
#include "PageCache.h"
//����ģʽ�Ķ�������һ�����.cpp��,�����Ժ���Ҫ��ȡ�ö���Ϳ��Ե���GetInstance()����ӿ�

CentralCache CentralCache::_inst;

Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	//��������ô֪�����Span�Ƿ���ʢ������Ҳû���ǲ���Ӧ��ȥPageCache��Ҫ������еĻ��Ƿ���ʣ��Ƿ����㹻�Ŀռ䣩
	//����spanlist��ȡ�һ����ڴ��span
	Span* it = list.Begin();
	while (it != list.End())
	{
		//��Span�е�memoryΪ�յ�ʱ�򣬾ͱ�ʾ������
		if (it->_list)
		{
			return it;
		}
		it = it->_next;
	}
	//�ߵ�������߼���ʵ�Ǹ�λ�þ�û��Span��Ҫô�����ҵ������Span������ڴ涼��������
	//����ߵ��������span��û���ڴ��ˣ���ô��ʱ��ֻ����PageCache
	//��ôҪ����ҳ�أ����Ҫ���ڴ��Ļ�����ô�ǲ���Ӧ�ö�Ҫ����ҳ�����Ҫ���ڴ�С�Ļ�����ô����1������2��ҳ�Ϳ���������

	//���ﷵ�ص�ҳӦ�����Ѿ����кõ�
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));

	//�зֺùҽ���list��
	//����Ҳ�Ϳ������CentralCache��PageCache�����ҽӵ�Span��ʲô��ͬ��CentralCache��span���кõ���һ����С�ڴ��Ѿ����ֳ�ȥ��
	//PageCache�е�spanһ�������Ĵ���ڴ棬������Ҫ�У�
	//�е�ʱ��Ӧ�õ�һ����һ�����span���ж��ٿ�С�ڴ�
	char* start = (char*)(span->_pageId << PAGE_SHIFT); //��������span����ʼ��ַ
	char* end = start + (span->_n << PAGE_SHIFT);
	//����ʹ��ͷ��,��һ�����Ϳ�����ȫ�İѸô��Span�и��
	while (start < end)
	{
		char* next = start + size;                            //start                       end
		NextObj(start) = span->_list;                         // ---------------------------
		// ---------------------------
		span->_list = start;

		start = next;
	}
	span->_objsize = size;
	list.PushFront(span); //��Ӧ�ð����span������������
	//�������и������С�ڴ治��size�Ĵ�С����Ӧ�ö�����      ����
	//������Ϊ�������4096 / 8   4096 / 16   4096 / 128    4096 / 1024 Ӧ�ö������������������Ͳ���Ҫ�ڿ���
	return span;

}

//����զ����Ҫ����size�Ĵ�С����ȡҳ������
//���ﷵ�ص���ʵ�ʵ�����
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t n, size_t size)
{
	//ֱ�������λ�ü����ǲ��õģ���Ϊ�ᵼ��ȡ��ͬ��spanList[i]���Ǵ��л���
	size_t i = SizeClass::Index(size);
	std::lock_guard<std::mutex> lock(_spanLists[i]._mtx);//RAII

	Span* span = GetOneSpan(_spanLists[i], size);

	// �ҵ�һ���ж����span���ж��ٸ�����
	size_t j = 1;
	start = span->_list;
	void* cur = start;
	void* prev = start;
	while (j <= n && cur != nullptr)
	{
		prev = cur;
		cur = NextObj(cur);
		++j;
		span->_usecount++; //��һ����Ϊ�黹����׼����
	}

	span->_list = cur;
	end = prev;
	NextObj(prev) = nullptr; //˳�����������ӹ�ϵҲ�ı䣬�����Ͳ�����������ԭ������һ����

	return j - 1;
}


void CentralCache::ReleaseListToSpans(void* start, size_t byte_size)
{
	//������һ�������������ܹ�֪�������С���ڴ���Ҫ����CentralCache��ʱ��ÿһ��С�ڴ�������һ��Span��
	//ԭ����������ʱ������ڴ������ڲ�ͬ��Span�����ҹ黹��˳���ǲ�ȷ����
	//�������ʱ�����ʹ��usecount��ʱ���ˣ���һ��Spanȫ������������ʱ�򣬾���Ҫ�黹��PageCache��һ�������PageCache�ϳɸ����ҳ

	//����������Ҫ����map��Ϊ�ľ����ܹ���ÿһ��С�ڴ��ҵ��������Ĵ��Span

	//��Ϊ����start>>12���õ��Ľ������ҳ�ţ���һ��ҳ������ĵ�ַ>>12�������ҳ�ţ�
	//�����Ϳ��԰Ѷ�Ӧ��С�ڴ滹����������Span
	size_t i = SizeClass::Index(byte_size);
	std::lock_guard<std::mutex> lock(_spanLists[i]._mtx);
	//һ��һ��������Ӧ��Span
	while (start)
	{
		//�������ǰ������Ҳ�����һ����
		void* next = NextObj(start);

		// ��start�ڴ�������ĸ�span
		Span* span = PageCache::GetInstance()->MapObjectToSpan(start);

		// �Ѷ�����뵽span�����list��
		NextObj(start) = span->_list;
		span->_list = start;
		span->_usecount--;
		// _usecount == 0˵�����span���г�ȥ�Ĵ���ڴ�
		// ����������
		//��Ҫ�����Spanɾ�������п������Span������λ�ã���������Ҳ�Ϳ�����SpanList���Ϊ˫���ͷѭ������
		if (span->_usecount == 0)
		{
			_spanLists[i].Erase(span);
			span->_list = nullptr; //��Span����ҽӵ�С�ڴ��ϵ���������Ϊ��PageCache����Ҫ����һ���������ڴ棬������Ҫ���и�õ�
			PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		}

		start = next;
	}

}
