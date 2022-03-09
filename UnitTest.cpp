#include "ObjectPool.h"
#include "ConcurrentAlloc.h"

void func1()
{
	for (size_t i = 0; i < 10; ++i)
	{
		ConcurrentAlloc(17);
	}
}

void func2()
{
	for (size_t i = 0; i < 20; ++i)
	{
		ConcurrentAlloc(5);
	}
}

void TestThreads()
{
	std::thread t1(func1);
	std::thread t2(func2);


	t1.join();
	t2.join();
}

void TestSizeClass()
{
	cout << SizeClass::Index(1035) << endl;
	cout << SizeClass::Index(1025) << endl;
	cout << SizeClass::Index(1024) << endl;
}

void TestConcurrentAlloc()
{
	/*void* ptr0 = ConcurrentAlloc(5); *///��δ��봦����һ�����εĴ���ģ���Ŀǰ���ԣ����һ��ʼ������8�ֽڣ���ô��PageCache������������ҳ
	//�ͻᱻ����8�ֽڶ��зֺ�Ȼ��ҽ���CentralCache��
	//������������һ��ʼ��ֻ������5�ֽ��أ���ô���ѵ�Ҫ��PageCache������������ҳ�г�һ����5�ֽڵ�С�ڴ�Ȼ��ҽ���CentralCache����
	//���Ի���Ҫһ����������������5�ֽڵ�ʱ��Ҳ���ɰ���8�ֽڽ�PageCache����������ҳ�����з�GetOneSpan()
	void* ptr1 = ConcurrentAlloc(8);
	void* ptr2 = ConcurrentAlloc(8);

	//ConcurrentFree(ptr0);
	ConcurrentFree(ptr1);
	ConcurrentFree(ptr2);
}

void TestBigMemory()
{
	void* ptr1 = ConcurrentAlloc(65 * 1024);
	ConcurrentFree(ptr1);

	//Ҳ�п����������һ�����128ҳ���ڴ�
	void* ptr2 = ConcurrentAlloc(129 * 4 * 1024);
	ConcurrentFree(ptr2);
}

//int main()
//{
//	//TestObjectPool();
//	//TestThreads();
//	//TestSizeClass();
//	TestConcurrentAlloc();
//	//TestBigMemory();
//	return 0;
//}