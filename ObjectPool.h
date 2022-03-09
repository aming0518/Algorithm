//ʵ��һ���������ڴ�أ����ĳһ������Ķ������������ֽ�ObjictPool��
#pragma once 

#include"Common.h"

template<class T>
class ObjectPool
{
public:
	~ObjectPool()
	{
		//
	}
	//��ʱ���뻹��һ���ܴ�����⣺����Ĭ������ȡ����ǰ�ĸ��ֽڣ�������64λ��ƽ̨�£���Ҫȡ��Ӧ�������С�ڴ��ǰ8���ֽ��������ַ
	void*& Nextobj(void* obj)
	{
		return *((void**)obj); //���ڷ��ص�void*�����Զ�������ƽ̨
	}
	//�����ڴ�ĺ����ӿ�
	T* New()
	{
		T* obj = nullptr;
		//һ��������Ӧ���ж�freeList
		if (_freeList)
		{
			//�Ǿ�ֱ�Ӵ�����������ȡһ�����
			obj = (T*)_freeList;
			//_freeList = (void*)(*(int*)_freeList);
			_freeList = Nextobj(_freeList);
		}
		else
		{
			//��ʾ���������ǿյ�
			//��ô������Ҫ�����жϣ�memory��û��
			if (_leftSize < sizeof(T)) //˵����ʱ�ռ䲻����
			{
				//��ô�ͽ����и�
				_leftSize = 1024 * 100;
				_memory = (char*)malloc(_leftSize);
				//����C++��˵�������ϵͳ����ʧ���ˣ�������쳣
				if (_memory == nullptr)
				{
					throw std::bad_alloc();
				}
			}
			//����memory���и�
			obj = (T*)_memory;
			_memory += sizeof(T); //��������벻ͨ���Ի�һ��ͼ���ܼ�
			_leftSize -= sizeof(T); //��ʾʣ��ռ�Ĵ�С
		}
		new(obj)T;  //��λnew����Ϊ������Ŀռ���������Զ���������û�г�ʼ����
		//������Ҫ������ʾ�ĵ���������͵Ĺ��캯���������ר������ڴ��ʹ�õ�
		return obj;
	}

	void Delete(T* obj)
	{
		obj->~T();//�Ȱ��Զ������ͽ�������
		//Ȼ���ڽ����ͷţ����Ǵ�ʱ�������Ķ���һ��һ���С�ڴ棬�޷�����һ���Խ���free��������Ҫһ������������ЩС�ڴ涼�ҽ�ס
		//������ʵ���Ǻ��ĵĹؼ���
		//����ָ����˵����32λ��ƽ̨������4�ֽڣ���64λƽ̨������8�ֽ�

		//ͷ�嵽freeList
		//*((int*)obj)= (int)_freeList;
		Nextobj(obj) = _freeList;
		_freeList = obj;
	}
private:
	char* _memory = nullptr;//�����char*��Ϊ�˺��ߴ�С��������һ��Ҫ��T*����void*
	int _leftSize = 0; //Ϊʲô����������Ա�����أ���Ϊ���menory += sizeof(T),�п��ܾͻ����Խ�������
	void* _freeList = nullptr; //��һЩȱʡֵ�������Ĺ��캯���Լ����ɾͿ�����
};

struct TreeNode
{
	int _val;
	TreeNode* _left;
	TreeNode* _right;

	TreeNode()
		:_val(0)
		, _left(nullptr)
		, _right(nullptr)
	{}
};
void TestObjectPool()
{

	//��֤���������ڴ��Ƿ��ظ����õ�����
	//	ObjectPool<TreeNode> tnPool;
	//TreeNode* node1 = tnPool.New();
	//TreeNode* node2 = tnPool.New();
	//cout << node1 << endl;
	//cout << node2 << endl;

	//tnPool.Delete(node1);
	//TreeNode* node3 = tnPool.New();
	//cout << node3 << endl;

	//cout << endl;

	//��֤�ڴ�ص��׿첻�죬��û���������ܵ��Ż�
	//new�ײ㱾����õ�malloc,��һֱ�Ͳ���ϵͳ�ĵײ��򽻵�
	size_t begin1 = clock();
	std::vector<TreeNode*> v1;
	for (int i = 0; i < 1000000; ++i)
	{
		v1.push_back(new TreeNode);
	}
	for (int i = 0; i < 1000000; ++i)
	{
		delete v1[i];
	}
	size_t end1 = clock();


	//�������ǵ����Լ���д���ڴ��
	ObjectPool<TreeNode> tnPool;
	size_t begin2 = clock();
	std::vector<TreeNode*> v2;
	for (int i = 0; i < 1000000; ++i)
	{
		v2.push_back(tnPool.New());
	}
	for (int i = 0; i < 1000000; ++i)
	{
		tnPool.Delete(v2[i]);
	}
	size_t end2 = clock();

	cout << end1 - begin1 << endl;
	cout << end2 - begin2 << endl;
}
