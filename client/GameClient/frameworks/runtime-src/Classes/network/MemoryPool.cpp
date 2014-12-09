#include "MemoryPool.h"

typedef unsigned int uint32;
typedef int			 int32;

//basic memory node structure
struct apr_memnode_t{
	uint32			magic;			//���ڱ������ڴ�����ڴ�������			    
	apr_memnode_t	*next;			//ָ����һ���ڴ�ռ�ڵ�
	apr_memnode_t	**ref;			//ָ��ǰ�ڴ�ռ�ڵ�
	uint32			index;			//��ǰ�ڴ�ռ�ڵ���ܹ��ڴ��С
	uint32			free_index;		//��ǰ�ڴ�ռ��еĿ��ÿռ�
	char*			first_avail;	//ָ��ǰ���ÿռ����ʼ��ַ����	������
	char*			endp;			//ָ��ǰ���õ�ַ�Ľ�����ַ��
};

class Allocator
{
public:
	enum{
		APR_ALLOCATOR_MAX_FREE_UNLIMITED = 0,	//���������������ڴ�û������
		DEFAULT_ALIGN = 8,						//���� 8 �ֽڶ���
		MAX_INDEX = 20,							//�����ڴ��б�Ľڵ�	
		BOUNDARY_INDEX = 12,					// �ڴ��Ķ��� index 
		BOUNDARY_SIZE =  (1 << BOUNDARY_INDEX), // �ڴ��Ķ����Сsize
		MIN_ALLOC = 2*BOUNDARY_SIZE,
	};
public:
	Allocator(size_t nMaxSize = APR_ALLOCATOR_MAX_FREE_UNLIMITED);
	virtual ~Allocator();
	inline const int GetMemNodeSize()
	{
		const int nMemNodeSize = Align(sizeof(apr_memnode_t), DEFAULT_ALIGN);
		return nMemNodeSize;
	}
	/*
	*	��õ�ǰ������������ڴ��ı��ֵ
	*/
	inline uint32 GetMagic()
	{
		return m_uiMagic;
	}
	/*
	*	��� nAllocSize �ռ��С�Ľڵ�
	*/
	apr_memnode_t*  Alloc(size_t nAllocSize);
	/*
	*	�ͷ�node�ڵ�Ŀռ䣬ע��������ͷŲ�һ����ֱ�Ӹ�ϵͳ����
	*	��������ʱ���ڷ������У����´�Ҫ�õ��ڴ�ʹ��
	*/
	void Free(apr_memnode_t *node);
private:
	/*
	*	����һ���ϴ���������
	*/
	static inline uint32	CreateMagic()
	{
		double start = 1, end = RAND_MAX;
		double uiMagic = (start + (end - start)*rand()/(RAND_MAX+1.0));
		uiMagic *= uiMagic;
		return (uint32)uiMagic;
	}
	/*
	*	function:	������ӽ�nSize �� nBoundary ������������������ð�ָ���ֽڶ����Ĵ�С
	*	parameter:	nSize Ϊ������ nBoundary������Ϊ 2 �ı���
	*	example:	Align(7�� 4) = 8��Align(21, 16) = 32
	*/
	static inline size_t Align(size_t nSize, size_t nBoundary)
	{
		return ((nSize +nBoundary-1) & ~(nBoundary - 1));
	}
	/*
	*	function:	���÷����ӵ�����ڴ����ռ����ƣ������ù�ϵ����
	*				�����������ж���ڴ�ʱ�Ὣ�ڴ淵�ظ�ϵͳ����
	*	paramter:	allocator : Ҫ���õķ����ӣ� nSize�� Ҫ���õĴ�С
	*	
	*/
	void inline SetMaxSize(size_t nSize)
	{
		uint32 uiMaxIndex = Align(nSize, BOUNDARY_SIZE) >> BOUNDARY_INDEX;
		
		//�����µ����ɴ�ſռ��С�������Ҫ��֤��ǰ m_uiCurAllocIndex(��ǰ�ɴ洢�ڷ������е��ڴ��С)
		//������ĵ�����������������ֵʱ��m_uiCurAllocIndex ==  m_uiMaxIndex ��Ҫ����Ӧ�����ӣ�
		//��� m_uiCurAllocIndex < m_uiMaxIndex ��ô���������ֵҲ����Ӱ�죬��Ϊ m_uiCurAllocIndex ���ں�����ʹ����
		//�ﵽ���ֵ��
		m_uiCurAllocIndex += uiMaxIndex - m_uiMaxIndex;
		m_uiMaxIndex = uiMaxIndex;

		if(m_uiCurAllocIndex > uiMaxIndex)
			m_uiCurAllocIndex = uiMaxIndex;
	}
	/*
	*	���������й��صĿռ�ȫ����ϵͳ����
	*/
	void Destroy();
private:
	uint32			m_uiMagic; //���ڼ�¼�η�����������ڴ��ı��ֵ
	uint32			m_uiCurMaxBlockIndex; //�������е�ǰ���õ�����ĵĴ�Сindex
	uint32			m_uiMaxIndex;//���������Դ洢�����ռ��Сindex
	uint32			m_uiCurAllocIndex;//��ǰ�Ѿ�����Ŀ����ڷ������еĿռ��С����ֵ������ m_uiMaxIndex��Χ��
	Mutex			m_mutex;		 //���̷߳�����
	apr_memnode_t	*m_pfree[MAX_INDEX];//��������ǰ���صĿ����ڴ��
};

/////////////////////////////////////////////////////////////////////////////////////////
//class Allocator public
Allocator::Allocator(size_t nMaxSize /*= APR_ALLOCATOR_MAX_FREE_UNLIMITED*/)
{
	m_uiMagic = CreateMagic();
	m_uiCurMaxBlockIndex = 0; //��ʼ״̬�£�m_pfree[] Ϊ�գ�����û�������ÿ� 
	m_uiMaxIndex = APR_ALLOCATOR_MAX_FREE_UNLIMITED;//��ʼ״̬Ϊ�ɴ洢�ռ�����
	m_uiCurAllocIndex = 0;//��ǰ�Ѿ�����Ŀ����ڷ������еĿռ��С����ֵ������ m_uiMaxIndex��Χ��
	memset(m_pfree, 0, sizeof(m_pfree));

	if(nMaxSize != APR_ALLOCATOR_MAX_FREE_UNLIMITED)
		SetMaxSize(nMaxSize);
}

Allocator::~Allocator()
{
	Destroy();
}
apr_memnode_t* Allocator::Alloc(size_t nAllocSize)
{
	apr_memnode_t *node, **ref;
	uint32 uiCurMaxBlockIndex;
	size_t nSize, i, index;

	const int nMemNodeSize = GetMemNodeSize();

	nSize = Align(nAllocSize + nMemNodeSize, BOUNDARY_SIZE);
	if(nSize < nAllocSize) //��������nAllocSize�����¼����nSize��nAllocSizeС
	{
		return NULL;
	}
	if(nSize < MIN_ALLOC)
		nSize = MIN_ALLOC;

	//������С��size = MIN_ALLOC = 2*BOUNDARY_SIZE ����index ��С����Ϊ 1
	index = (nSize >> BOUNDARY_INDEX) - 1;
	if(index > UINT32_MAX) //����Ŀռ�����򲻷���
	{
		return NULL;
	}
	if(index <= m_uiCurMaxBlockIndex)//��ǰ���ڿ��õ��ڴ�鹻index
	{
		m_mutex.lock();

		uiCurMaxBlockIndex = m_uiCurMaxBlockIndex;
		ref = &m_pfree[index];
		i = index;
		while(NULL == *ref && i < uiCurMaxBlockIndex)
			ref++, i++;

		if(NULL != (node = *ref))
		{
			//����ҵ��Ŀ����ڴ���ǵ�ǰ�����������Ŀ飬�������һ������
			//����·������е�ǰ�Ŀ�������
			if(NULL == (*ref = node->next) && i >= uiCurMaxBlockIndex)
			{
				do{
					ref--;
					uiCurMaxBlockIndex--;
				}while(NULL == *ref && uiCurMaxBlockIndex > 0);
				m_uiCurMaxBlockIndex = uiCurMaxBlockIndex;
			}

			m_uiCurAllocIndex += node->index + 1;
			if(m_uiCurAllocIndex > m_uiMaxIndex)
				m_uiCurAllocIndex = m_uiMaxIndex;

			m_mutex.unlock();
			node->next = NULL;
			node->first_avail= (char*)node + nMemNodeSize;
			return node;
		}
		m_mutex.unlock();
	}
	else if(m_pfree[0])//����п��õĴ��ڴ���ڿ��õĴ��ڴ����Ѱ��
	{
		m_mutex.lock();
		ref = &m_pfree[0];
		while(NULL != (node = *ref) && index > node->index)
			ref = &node->next;

		if(node)
		{
			*ref = node->next;
			m_uiCurAllocIndex += node->index + 1;
			if(m_uiCurAllocIndex > m_uiMaxIndex)
				m_uiCurAllocIndex = m_uiMaxIndex;

			m_mutex.unlock();
			node->next = NULL;
			node->first_avail = (char*)node + nMemNodeSize;
			return node;
		}
		m_mutex.unlock();
	}

	//����������ڴ�ʧ��
	if(NULL == (node = (apr_memnode_t*)malloc(nSize)))
	{
		return NULL;
	}

	node->magic = m_uiMagic;
	node->next = NULL;
	node->index = index;
	node->first_avail = (char*)node + nMemNodeSize;
	node->endp = (char*)node + nSize;

	return node;
}
void Allocator::Free(apr_memnode_t *node)
{
	apr_memnode_t *next, *freelist = NULL;
	uint32 index, uiCurMaxBlockIndex;
	uint32 uiMaxIndex, uiCurAllocIndex;

	m_mutex.lock();
	uiCurMaxBlockIndex = m_uiCurMaxBlockIndex;
	uiMaxIndex = m_uiMaxIndex;
	uiCurAllocIndex = m_uiCurAllocIndex;

	do{
		next = node->next;
		index = node->index;

		if(APR_ALLOCATOR_MAX_FREE_UNLIMITED != uiMaxIndex
			&& index + 1 > uiCurAllocIndex) //�����ǰindex + 1 �ռ��ǳ����޶�maxindex �Ŀռ�����ɾ��
		{
			node->next = freelist;
			freelist = node;
		}
		else if(index < MAX_INDEX)
		{
			if(NULL == (node->next = m_pfree[index])
				&& index > uiCurMaxBlockIndex)
			{
				uiCurMaxBlockIndex = index;
			}
			m_pfree[index] = node;
			if(uiCurAllocIndex >= index + 1)
				uiCurAllocIndex -= index + 1;
			else
				uiCurAllocIndex = 0;
		}
		else
		{
			node->next = m_pfree[0];
			m_pfree[0] = node;
			if(uiCurAllocIndex >= index + 1)
				uiCurAllocIndex -= index + 1;
			else
				uiCurAllocIndex = 0;

		}
	}while(NULL != (node = next));
	m_uiCurMaxBlockIndex = uiCurMaxBlockIndex;
	m_uiCurAllocIndex = uiCurAllocIndex;

	m_mutex.unlock();
	while(NULL != freelist)
	{
		node = freelist;
		freelist = node->next;
		free(node);
	}
}
void Allocator::Destroy()
{
	uint32 index;
	apr_memnode_t *node, **ref;

	for(index = 0; index < MAX_INDEX; index++)
	{
		ref = &m_pfree[index];
		while((node = *ref) != NULL){
			*ref = node->next;
			free(node);
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
//class MemoryPool public
MemoryPool::MemoryPool(size_t nMaxSize /*= Allocator::APR_ALLOCATOR_MAX_FREE_UNLIMITED*/)
{
	m_pAllocator = new Allocator(nMaxSize);
}
void* MemoryPool::Alloc(size_t nAllocaSize)
{
	apr_memnode_t* node = m_pAllocator->Alloc(nAllocaSize);
	if(node == NULL)
	{
		return NULL;
	}
	return node->first_avail;
}
bool MemoryPool::Free(void* pMem)
{
	if(NULL == pMem)
	{
		return false;
	}
	apr_memnode_t* node = (apr_memnode_t*)((char*)pMem - m_pAllocator->GetMemNodeSize());
	if(node->magic != m_pAllocator->GetMagic()) //����˶��ڴ治�Ǵ��ڴ�صķ����������
	{
		return false;
	}
	m_pAllocator->Free(node);
	return true;
}
MemoryPool::~MemoryPool()
{
	if(m_pAllocator)
		delete m_pAllocator;
}