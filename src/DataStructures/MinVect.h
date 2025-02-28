#ifndef MINVECT_H_
#define MINVECT_H_

class ISerializable
{
public:
    virtual unsigned int GetSerializedLen() = 0;
    virtual unsigned char* Serialize(unsigned int* a_nrbytes = 0) = 0;
    virtual unsigned int Deserialize(unsigned char* a_buffer) = 0;
};

template <typename VType>
class CVector: public ISerializable
{
public:
    unsigned int m_real_size;
    VType* m_data;
    unsigned int m_size;
    CVector()
    {
        m_data = 0;
        m_size = 0;
        m_real_size = 0;
    }

    CVector(unsigned int a_size)
    {
        m_data = 0;
        m_size = 0;
        m_real_size = 0;
        if (a_size)
            Rebuild(a_size);
    }

    virtual ~CVector()
    {
        if (m_data && m_real_size)
            delete[] m_data;
    }

    void Rebuild(unsigned int a_size = 0)
    {
        if (m_data && m_real_size)
            delete[] m_data;
        if (a_size)
        {
            m_data = new VType[a_size];
            m_real_size = a_size;
            m_size = a_size;
        }
        else
        {
            m_data = 0;
            m_real_size = 0;
            m_size = 0;
        }
    }

    virtual void RebuildPreserve(unsigned int a_size)
    {
        if (!a_size)
        {
            if (m_data)
                delete[] m_data;
            m_real_size = 0;
            m_size = 0;
            m_data = 0;
        }
        else if (!m_real_size)
        {
            Rebuild(a_size);
        }
        else if (a_size <= m_real_size)
        {
            m_size = a_size;
        }
        else
        {
            unsigned int tLastSize = m_size;
            m_real_size = NearestPow2(a_size);
            m_size = a_size;
            VType* l_data = new VType[m_real_size];
            memcpy(l_data, m_data, tLastSize * sizeof(VType));
            delete[] m_data;
            m_data = l_data;
        }
    }

    virtual void AddElement(const VType a_value)
    {
        if (m_size >= m_real_size)
        {
            RebuildPreserve(m_size + 1);
            m_data[m_size - 1] = a_value;
        }
        else
            m_data[m_size++] = a_value;
    }

    virtual void AddElement(const VType* a_value)
    {
        if (m_size >= m_real_size)
        {
            RebuildPreserve(m_size + 1);
            memcpy(&m_data[m_size - 1], a_value, sizeof(VType));
        }
        else
            memcpy(&m_data[m_size++], a_value, sizeof(VType));
    }

    void RemoveElement(int a_offset)
    {
        RemoveElements(a_offset);
    }

    virtual void RemoveElements(unsigned int a_offset, unsigned int a_nrelements = 1)
    {
        if (!m_size)
            return;
        if (a_nrelements < 1)
            return;
        if ((a_nrelements + a_offset) > m_size)
            return;
        if (a_offset >= m_size)
            return;
        if ((a_offset == 0) && (a_nrelements == m_size))
        {
            if (m_data && m_size)
                delete[] m_data;
            m_data = 0;
            m_size = 0;
            m_real_size = 0;
            return;
        }

        VType* l_data = new VType[m_size];
        memcpy(l_data, m_data, m_size * sizeof(VType));

        m_size -= a_nrelements;
        if (a_offset)
            memcpy(m_data, l_data, a_offset * sizeof(VType));
        if (a_offset < m_size)
            memcpy(m_data + a_offset, l_data + a_offset + a_nrelements, (m_size - a_offset) * sizeof(VType));

        delete[] l_data;
    }

    void SearchAndRemove(VType value, int a_nrelements)
    {
        if (!m_size)
            return;
restartit:
        if (a_nrelements)
            for (unsigned int i = 0; i < m_size; ++i)
                if (m_data[i] == value)
                {
                    RemoveElement(i);
                    a_nrelements--;
                    goto restartit;
                }
    }

    void AddIfNotAddedYet(const VType a_value, int a_offset)
    {
        for (unsigned int i = 0; i < m_size; ++i)
            if (m_data[i] == a_value)
                return;
        AddElement(a_value);
    }

    virtual unsigned int GetSerializedLen()
    {
        unsigned int buffsiz = 8;
        if ((m_data) && (m_size))
            buffsiz += m_size * sizeof(VType);
        return buffsiz;
    }

    virtual unsigned char* Serialize(unsigned int* a_nrbytes = 0)
    {
        unsigned int buffsiz = CVector<VType>::GetSerializedLen();
        unsigned char* buff1 = 0;

        buff1 = new unsigned char[buffsiz];
        *((unsigned int*)buff1) = buffsiz;
        *((unsigned int*)&buff1[4]) = m_size;
        if ((m_data) && (m_size))
            memcpy(buff1 + 8, m_data, m_size * sizeof(VType));
        if (a_nrbytes)
            *a_nrbytes = buffsiz;
        return buff1;
    }

    virtual unsigned int Deserialize(unsigned char* a_buffer)
    {
        if (m_data)
            delete[] m_data;
        m_size = 0;
        m_data = 0;
        m_real_size = 0;
        if (a_buffer)
        {
            m_size = *((unsigned int*)&a_buffer[4]);
            if (m_size)
            {
                m_data = new VType[m_size];
                memcpy(m_data, a_buffer + 8, m_size * sizeof(VType));
            }
        }
        unsigned int result = 0;
        if (a_buffer)
            result = m_size * sizeof(VType) + 8;
        m_real_size = m_size;
        return result;
    }

    CVector& operator = (const CVector& a_vect)
    {
        Rebuild(a_vect.m_size);
        memcpy(m_data, a_vect.m_data, m_size * sizeof(VType));
        return *this;
    }

    CVector(const CVector& rhs)
    {
        Rebuild(rhs.m_size);
        memcpy(m_data, rhs.m_data, m_size * sizeof(VType));
    }
};

template <typename VTypeBase>
class CList: public CVector<VTypeBase>
{
public:
    template <typename DerivedMemberFunc>
    void operator += (DerivedMemberFunc a_value)
    {
        CVector<VTypeBase>::AddIfNotAddedYet((VTypeBase)a_value, CVector<VTypeBase>::m_size);
    }

    template <typename DerivedMemberFunc>
    void operator -= (DerivedMemberFunc a_value)
    {
        CVector<VTypeBase>::SearchAndRemove((VTypeBase)a_value, 1);
    }

    template <typename DerivedMemberFunc>
    void operator = (DerivedMemberFunc a_value)
    {
        CVector<VTypeBase>::Rebuild();
        if (a_value)
            AddIfNotAddedYet((VTypeBase)a_value, CVector<VTypeBase>::m_size);
    }
};

template <typename VTypeBase>
class CVectorRS: public CVector<VTypeBase*>
{
public:
    virtual unsigned int GetSerializedLen()
    {
        unsigned int buffsiz = 8;
        if ((CVector<VTypeBase*>::m_data) && (CVector<VTypeBase*>::m_size))
        {
            for (unsigned int i = 0; i < CVector<VTypeBase*>::m_size; ++i)
                buffsiz += CVector<VTypeBase*>::m_data[i]->GetSerializedLen();
        }
        return buffsiz;
    }

    virtual unsigned char* Serialize(unsigned int* a_nrbytes = 0)
    {
        unsigned int buffsiz = CVectorRS<VTypeBase>::GetSerializedLen();
        unsigned char* buff1 = 0;

        buff1 = new unsigned char[buffsiz];
        *((unsigned int*)buff1) = buffsiz;
        *((unsigned int*)&buff1[4]) = CVector<VTypeBase*>::m_size;

        if ((CVector<VTypeBase*>::m_data) && (CVector<VTypeBase*>::m_size))
        {
            unsigned int buff1indx = 8;
            for (unsigned int i = 0; i < CVector<VTypeBase*>::m_size; ++i)
            {
                unsigned int actualbufflen;
                unsigned char*tembuff = CVector<VTypeBase*>::m_data[i]->Serialize(&actualbufflen);
                if (tembuff)
                {
                    memcpy(buff1 + buff1indx, tembuff, actualbufflen);
                    buff1indx += actualbufflen;
                    delete[] tembuff;
                }
            }
        }

        if (a_nrbytes)
            *a_nrbytes = buffsiz;
        return buff1;
    }

    void Rebuild(unsigned int a_nrlements)
    {
        if ((CVector<VTypeBase*>::m_data) && (CVector<VTypeBase*>::m_size))
        {
            for (unsigned int i = 0; i < CVector<VTypeBase*>::m_size; ++i)
                delete CVector<VTypeBase*>::m_data[i];
            delete[] CVector<VTypeBase*>::m_data;
        }
        if (a_nrlements)
        {
            CVector<VTypeBase*>::m_data = new VTypeBase* [a_nrlements];
            CVector<VTypeBase*>::m_size = a_nrlements;
            CVector<VTypeBase*>::m_real_size = a_nrlements;
            for (unsigned int i = 0; i < CVector<VTypeBase*>::m_size; ++i)
                CVector<VTypeBase*>::m_data[i] = new VTypeBase;
        }
        else
        {
            CVector<VTypeBase*>::m_data = 0;
            CVector<VTypeBase*>::m_size = 0;
            CVector<VTypeBase*>::m_real_size = 0;
        }
    }

    void RebuildPreserve(unsigned int a_nrlements)
    {
        if (!a_nrlements)
        {
            Rebuild(0);
            return;
        }
        if (!CVector<VTypeBase*>::m_size)
        {
            Rebuild(a_nrlements);
            return;
        }
        if (a_nrlements == CVector<VTypeBase*>::m_size)
            return;
        VTypeBase** l_data = 0;
        if (a_nrlements < CVector<VTypeBase*>::m_size)
        {
            for (int i = a_nrlements; i < (int)CVector<VTypeBase*>::m_size; ++i)
                delete CVector<VTypeBase*>::m_data[i];
            CVector<VTypeBase*>::m_size = a_nrlements;
            CVector<VTypeBase*>::m_real_size = a_nrlements;
            l_data = new VTypeBase*[CVector<VTypeBase*>::m_size];
            memcpy(l_data, CVector<VTypeBase*>::m_data, CVector<VTypeBase*>::m_size * sizeof(VTypeBase*));
            delete[] CVector<VTypeBase*>::m_data;
            CVector<VTypeBase*>::m_data = new VTypeBase*[CVector<VTypeBase*>::m_size];
            memcpy(CVector<VTypeBase*>::m_data, l_data, CVector<VTypeBase*>::m_size * sizeof(VTypeBase*));
        }
        if (a_nrlements > CVector<VTypeBase*>::m_size)
        {
            l_data = new VTypeBase*[CVector<VTypeBase*>::m_size];
            memcpy(l_data, CVector<VTypeBase*>::m_data, CVector<VTypeBase*>::m_size * sizeof(VTypeBase*));
            delete[] CVector<VTypeBase*>::m_data;
            unsigned int lastsize = CVector<VTypeBase*>::m_size;
            CVector<VTypeBase*>::m_size = a_nrlements;
            CVector<VTypeBase*>::m_real_size = a_nrlements;
            CVector<VTypeBase*>::m_data = new VTypeBase*[CVector<VTypeBase*>::m_size];
            memcpy(CVector<VTypeBase*>::m_data, l_data, lastsize * sizeof(VTypeBase*));
            for (unsigned int i = lastsize; i < CVector<VTypeBase*>::m_size; ++i)
                CVector<VTypeBase*>::m_data[i] = new VTypeBase;
        }
        if (l_data)
            delete[] l_data;
    }

    virtual int Deserialize(unsigned char* a_buffer, unsigned int* a_nrbytes = 0)
    {
        int result = 0;
        unsigned int a_nrlements = 0;
        if (a_buffer)
            a_nrlements = *((unsigned int*)&a_buffer[4]);
        Rebuild(a_nrlements);
        unsigned int buff1indx = 8;
        if (a_buffer)
        {
            for (unsigned int i = 0; i < CVector<VTypeBase*>::m_size; ++i)
            {
                int actualbufflen = CVector<VTypeBase*>::m_data[i]->Deserialize(a_buffer + buff1indx);
                buff1indx += actualbufflen;
            }
            result = true;
        }
        if (a_nrbytes)
        {
            if (a_buffer)
                *a_nrbytes = buff1indx;
            else
                *a_nrbytes = 0;
        }
        return result;
    }

    virtual ~CVectorRS()
    {
        if ((CVector<VTypeBase*>::m_data) && (CVector<VTypeBase*>::m_size))
            for (unsigned int i = 0; i < CVector<VTypeBase*>::m_size; ++i)
                delete CVector<VTypeBase*>::m_data[i];
    }
};

template <typename VTypeBase>
class CListRS: public CVectorRS<VTypeBase>
{
public:
    template <typename DerivedMemberFunc>
    void operator += (DerivedMemberFunc a_value)
    {
        AddIfNotAddedYet((VTypeBase)a_value);
    }

    template <typename DerivedMemberFunc>
    void operator -= (DerivedMemberFunc a_value)
    {
        SearchAndRemove((VTypeBase)a_value);
    }

    template <typename DerivedMemberFunc>
    void operator = (DerivedMemberFunc a_value)
    {
        CVectorRS<VTypeBase>::Rebuild();
        if (a_value)
            AddIfNotAddedYet((VTypeBase)a_value);
    }
};

template <typename VType>
class CMatrix: public IFileIO
{
public:
    VType** m_data;
    unsigned int m_width;
    unsigned int m_height;
    CMatrix()
    {
        Initialize();
    }

    CMatrix(int a_height, int a_width)
    {
        Initialize();
        Rebuild(a_height, a_width);
    }

    ~CMatrix()
    {
        Deinitialize();
    }

    void Initialize()
    {
        m_data   = 0;
        m_width  = 0;
        m_height = 0;
    }

    void Deinitialize()
    {
        if (m_data)
        {
            delete[] m_data[0];
            delete[] m_data;
            m_data   = 0;
            m_width  = 0;
            m_height = 0;
        }
    }

    void Rebuild(int a_height, int a_width)
    {
        Deinitialize();
        if ((!a_width) || (!a_height))
            return;

        m_width  = a_width;
        m_height = a_height;
        m_data = new VType* [a_height];
        VType* rawdat = new VType  [a_height * a_width];
        for (int i = 0; i < a_height; ++i)
            m_data[i] = &rawdat[i * a_width];
    }

    virtual int GetSerializedLen()
    {
        int buffsiz = 12;
        if ((m_data) && (m_width) && (m_height))
            buffsiz += m_width * m_height * sizeof(VType);
        return buffsiz;
    }

    virtual byte* Serialize(unsigned int* a_nrbytes = 0)
    {
        int buffsiz = CMatrix<VType>::GetSerializedLen();
        byte* buff1 = new byte[buffsiz];
        *((int*)buff1) = buffsiz;
        *((int*)&buff1[4]) = m_width;
        *((int*)&buff1[8]) = m_height;
        if ((m_data) && (m_width) && (m_height))
            memcpy(buff1 + 12, m_data[0], m_width * m_height * sizeof(VType));
        if (a_nrbytes)
            *a_nrbytes = buffsiz;
        return buff1;
    }

    virtual bool Deserialize(byte* a_buffer, int* a_nrbytes = 0)
    {
        bool result = 0;
        Deinitialize();
        if (a_buffer)
        {
            m_width = *((int*)&a_buffer[4]);
            m_height = *((int*)&a_buffer[8]);

            Rebuild(m_width, m_height);
            if ((m_width) && (m_height))
                memcpy(m_data[0], a_buffer + 12, m_width * m_height * sizeof(VType));
            result = true;
        }
        if (a_nrbytes)
        {
            if (a_buffer)
                *a_nrbytes = GetSerializedLen();
            else
                *a_nrbytes = 0;
        }
        return result;
    }
};

template <typename VType>
class CMatrixVarLen: public IFileIO
{
public:
    VType** m_data;
    CVector<unsigned int> m_widths;
    CMatrixVarLen()
    {
        Initialize();
    }

    CMatrixVarLen(int a_height, unsigned int* a_widths)
    {
        Initialize();
        RebuildPreserve(a_height, a_widths);
    }

    ~CMatrixVarLen()
    {
        Deinitialize();
    }

    void Initialize()
    {
        m_data = 0;
    }

    void Deinitialize()
    {
        if (m_data)
        {
            delete[] m_data[0];
            delete[] m_data;
            m_data = 0;
            m_widths.Rebuild(0);
        }
    }

    int GetTotalNrElements()
    {
        int total = 0;
        if (m_data)
            for (unsigned int i = 0; i < m_widths.m_size; ++i)
                total += m_widths.m_data[i];
        return total;
    }

    virtual void RebuildPreserve(int a_height, unsigned int* a_widths)
    {
        CMatrixVarLen<VType>temmx;
        temmx.Rebuild(a_height, a_widths);
        unsigned int minheight = a_height;
        if (minheight > m_widths.m_size)
            minheight = m_widths.m_size;
        for (unsigned int i = 0; i < minheight; ++i)
        {
            unsigned int minwidth = a_widths[i];
            if (minwidth > m_widths.m_data[i])
                minwidth = m_widths.m_data[i];
            memcpy(temmx.m_data[i], m_data[i], minwidth * sizeof(VType));
            //if (a_widths[i] > m_widths.m_data[i])
                //memset(temmx.m_data[i] + m_widths.m_data[i] - 1, 0, (a_widths[i] - m_widths.m_data[i]) * sizeof(VType));
        }
        //for (int i = m_widths.m_size; i < a_height; ++i)
          //  memset(temmx.m_data[i], 0, temmx.m_widths.m_data[i] * sizeof(VType));
        CMatrixVarLen<VType>::Rebuild(a_height, a_widths);
        if (a_height)
            memcpy(m_data[0], temmx.m_data[0], GetTotalNrElements() * sizeof(VType));
    }

    virtual void CopyFrom(CMatrixVarLen<VType>* a_mx)
    {
        Rebuild(a_mx->m_widths.m_size, a_mx->m_widths.m_data);
        memcpy(m_data[0], a_mx->m_data[0], GetTotalNrElements() * sizeof(VType));
    }

    virtual void Rebuild(int a_height, unsigned int* a_widths)
    {
        Deinitialize();
        if ((!a_widths) || (!a_height))
            return;
        m_widths.Rebuild(a_height);
        for (unsigned int i = 0; i < m_widths.m_size; ++i)
            m_widths.m_data[i] = a_widths[i];

        m_data = new VType* [m_widths.m_size];
        VType* rawdat = new VType  [GetTotalNrElements()];
        memset(rawdat, 0, GetTotalNrElements() * sizeof(VType));
        int rowdatindx = 0;
        for (unsigned int i = 0; i < m_widths.m_size; ++i)
        {
            m_data[i] = rawdat + rowdatindx;
            rowdatindx += m_widths.m_data[i];
        }
    }

    virtual int GetSerializedLen()
    {
        int buffsiz = sizeof(int) * 1;
        buffsiz += m_widths.GetSerializedLen();
        buffsiz += GetTotalNrElements() * sizeof(VType);
        return buffsiz;
    }

    virtual byte* Serialize(unsigned int* a_nrbytes = 0)
    {
        int buffsiz = CMatrixVarLen<VType>::GetSerializedLen();
        CBuffer l_buffer;
        l_buffer.Create(buffsiz);
        l_buffer.CatBuffer((byte*)&buffsiz, sizeof(int));
        unsigned int tmplen = 0;
        unsigned char* tmp = Serialize(&tmplen);
        l_buffer.CatBuffer(tmp, tmplen);
        delete[] tmp;
        int nrtotalelements = GetTotalNrElements();

        if (nrtotalelements)
            l_buffer.CatBuffer((byte*)m_data[0], nrtotalelements * sizeof(VType));
        if (a_nrbytes)
            *a_nrbytes = nrtotalelements * sizeof(VType);
        return l_buffer.GetBuffer();
    }

    virtual bool Deserialize(byte* a_buffer, int* a_nrbytes = 0)
    {
        bool result = false;
        if (a_buffer)
        {
            CVector <unsigned int> temm_widths;
            int temwidthsbytes;
            int rowdatindx = 4;
            temwidthsbytes = temm_widths.Deserialize(a_buffer + rowdatindx);
            rowdatindx += temwidthsbytes;
            Rebuild(temm_widths.m_size, temm_widths.m_data);

            for (unsigned int i = 0; i < m_widths.m_size; ++i)
                if (m_widths.m_data[i])
                {
                    memcpy(m_data[i], a_buffer + rowdatindx, m_widths.m_data[i] * sizeof(VType));
                    rowdatindx += m_widths.m_data[i] * sizeof(VType);
                }
            result = true;
        }
        if (a_nrbytes)
        {
            if (a_buffer)
                *a_nrbytes = GetSerializedLen();
            else
                *a_nrbytes = 0;
        }
        return result;
    }
};

#endif // MINVECT_H_
