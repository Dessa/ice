// **********************************************************************
//
// Copyright (c) 2003-2012 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef ICE_STREAM_H
#define ICE_STREAM_H

#include <Ice/StreamF.h>
#include <Ice/CommunicatorF.h>
#include <Ice/Object.h>
#include <Ice/Exception.h>
#include <Ice/Proxy.h>
#include <Ice/SlicedDataF.h>
#include <IceUtil/Shared.h>
#include <Ice/StreamTraits.h>

namespace Ice
{

class ICE_API ReadObjectCallback : public ::IceUtil::Shared
{
public:

    virtual void invoke(const ObjectPtr&) = 0;
};
typedef IceUtil::Handle<ReadObjectCallback> ReadObjectCallbackPtr;

template<typename T>
class ReadObjectCallbackI : public ReadObjectCallback
{

public:

    ReadObjectCallbackI(::IceInternal::Handle<T>& v) :
        _v(v)
    {
    }

    virtual void invoke(const ObjectPtr& p)
    {
        _v = ::IceInternal::Handle<T>::dynamicCast(p);
        if(p && !_v)
        {
            IceInternal::Ex::throwUOE(T::ice_staticId(), p);
        }
    }

private:

    ::IceInternal::Handle<T>& _v;
};

class ICE_API UserExceptionReader : public UserException
{
public:

    UserExceptionReader(const CommunicatorPtr&);
    ~UserExceptionReader() throw();

    virtual void read(const InputStreamPtr&) const = 0;
    virtual bool usesClasses() const = 0;
    virtual void usesClasses(bool) = 0;

    virtual ::std::string ice_name() const = 0;
    virtual UserExceptionReader* ice_clone() const = 0;
    virtual void ice_throw() const = 0;

    virtual void __write(IceInternal::BasicStream*) const;
    virtual void __read(IceInternal::BasicStream*);

    virtual bool __usesClasses() const;
    virtual void __usesClasses(bool);

protected:

    const CommunicatorPtr _communicator;
};

class ICE_API UserExceptionReaderFactory : public IceUtil::Shared
{
public:

    virtual void createAndThrow(const std::string&) const = 0;
};
typedef ::IceUtil::Handle<UserExceptionReaderFactory> UserExceptionReaderFactoryPtr;

class ICE_API InputStream : public ::IceUtil::Shared
{
public:

    virtual CommunicatorPtr communicator() const = 0;

    virtual void sliceObjects(bool) = 0;

    //
    // Sequences of bool are handled specifically because C++
    // optimizations for vector<bool> prevent us from reading vector
    // of bools the same way as other sequences.
    //
    void 
    read(std::vector<bool>& v)
    {
        Int sz = readAndCheckSeqSize(StreamTrait<bool>::minWireSize);
        v.resize(sz);
        for(std::vector<bool>::iterator p = v.begin(); p != v.end(); ++p)
        {
            //
            // We can't just call inS->read(*p) here because *p is
            // a compiler dependent bit reference.
            //
            bool b;
            read(b);
            *p = b;
        }
    }

    virtual Int readSize() = 0;
    virtual Int readAndCheckSeqSize(int) = 0;

    virtual ObjectPrx readProxy() = 0;
    template<typename T> void read(IceInternal::ProxyHandle<T>& v)
    {
        ObjectPrx proxy = readProxy();
        if(!proxy)
        {
            v = 0;
        }
        else
        {
            v = new T;
            v->__copyFrom(proxy);
        }
    }

    virtual void readObject(const ReadObjectCallbackPtr&) = 0;
    template<typename T> void read(IceInternal::Handle<T>& v)
    {
        readObject(new ReadObjectCallbackI<T>(v));
    }

    Int
    readEnum(Int limit)
    {
        if(getEncoding() == Encoding_1_0)
        {
            if(limit <= 127)
            {
                Byte value;
                read(value);
                return value;
            }
            else if(limit <= 32767)
            {
                Short value;
                read(value);
                return value;
            }
            else 
            {
                Int value;
                read(value);
                return value;
            }
        }
        else
        {
            return readSize();
        }
    }

    virtual void throwException() = 0;
    virtual void throwException(const UserExceptionReaderFactoryPtr&) = 0;

    virtual void startObject() = 0;
    virtual SlicedDataPtr endObject(bool) = 0;

    virtual void startException() = 0;
    virtual SlicedDataPtr endException(bool) = 0;

    virtual std::string startSlice() = 0;
    virtual void endSlice() = 0;
    virtual void skipSlice() = 0;

    virtual Ice::EncodingVersion startEncapsulation() = 0;
    virtual void endEncapsulation() = 0;
    virtual Ice::EncodingVersion skipEncapsulation() = 0;

    virtual Ice::EncodingVersion getEncoding() = 0;

    virtual void readPendingObjects() = 0;

    virtual void rewind() = 0;

    virtual void skip(Ice::Int) = 0;
    virtual void skipSize() = 0;

    virtual void read(bool&) = 0;
    virtual void read(Byte&) = 0;
    virtual void read(Short&) = 0;
    virtual void read(Int&) = 0;
    virtual void read(Long&) = 0;
    virtual void read(Float&) = 0;
    virtual void read(Double&) = 0;
    virtual void read(::std::string&, bool = true) = 0;
    virtual void read(::std::vector< ::std::string>&, bool) = 0; // Overload required for additional bool argument.
    virtual void read(::std::wstring&) = 0;

    virtual void read(::std::pair<const bool*, const bool*>&, ::IceUtil::ScopedArray<bool>&) = 0;
    virtual void read(::std::pair<const Byte*, const Byte*>&) = 0;
    virtual void read(::std::pair<const Short*, const Short*>&, ::IceUtil::ScopedArray<Short>&) = 0;
    virtual void read(::std::pair<const Int*, const Int*>&, ::IceUtil::ScopedArray<Int>&) = 0;
    virtual void read(::std::pair<const Long*, const Long*>&, ::IceUtil::ScopedArray<Long>&) = 0;
    virtual void read(::std::pair<const Float*, const Float*>&, ::IceUtil::ScopedArray<Float>&) = 0;
    virtual void read(::std::pair<const Double*, const Double*>&, ::IceUtil::ScopedArray<Double>&) = 0;

    virtual bool readOptional(Int, OptionalType) = 0; 

    template<typename T> inline void read(T& v)
    {
        StreamHelper<T, StreamTrait<T>::type>::read(this, v);
    }

    template<typename T> inline void read(Ice::Int tag, IceUtil::Optional<T>& v)
    {
        if(readOptional(tag, StreamOptionalHelper<T, 
                                                  Ice::StreamTrait<T>::type, 
                                                  Ice::StreamTrait<T>::optionalType>::optionalType))
        {
            v.__setIsSet();
            StreamOptionalHelper<T, Ice::StreamTrait<T>::type, Ice::StreamTrait<T>::optionalType>::read(this, *v);
        }
    }

    virtual void closure(void*) = 0;
    virtual void* closure() const = 0;
};

class ICE_API OutputStream : public ::IceUtil::Shared
{
public:

    typedef size_t size_type;

    virtual CommunicatorPtr communicator() const = 0;

    virtual void writeSize(Int) = 0;

    virtual void writeProxy(const ObjectPrx&) = 0;
    template<typename T> void write(const IceInternal::ProxyHandle<T>& v)
    {
        writeProxy(ObjectPrx(upCast(v.get())));
    }

    virtual void writeObject(const ObjectPtr&) = 0;
    template<typename T> void write(const IceInternal::Handle<T>& v)
    {
        writeObject(ObjectPtr(upCast(v.get())));
    }

    void
    writeEnum(Int v, Int limit)
    {
        if(getEncoding() == Encoding_1_0)
        {
            if(limit <= 127)
            {
                write(static_cast<Byte>(v));
            }
            else if(limit <= 32767)
            {
                write(static_cast<Short>(v));
            }
            else 
            {
                write(v);
            }
        }
        else
        {
            writeSize(v);
        }
    }

    virtual void writeException(const UserException&) = 0;

    virtual void startObject(const SlicedDataPtr&) = 0;
    virtual void endObject() = 0;

    virtual void startException(const SlicedDataPtr&) = 0;
    virtual void endException() = 0;

    virtual void startSlice(const ::std::string&, bool) = 0;
    virtual void endSlice() = 0;

    virtual void startEncapsulation(const Ice::EncodingVersion&, FormatType) = 0;
    virtual void startEncapsulation() = 0;
    virtual void endEncapsulation() = 0;

    virtual Ice::EncodingVersion getEncoding() = 0;

    virtual void writePendingObjects() = 0;

    virtual void finished(::std::vector<Byte>&) = 0;

    virtual size_type pos() = 0;
    virtual void rewrite(Int, size_type) = 0;

    virtual void reset(bool) = 0;

    virtual void write(bool) = 0;
    virtual void write(Byte) = 0;
    virtual void write(Short) = 0;
    virtual void write(Int) = 0;
    virtual void write(Long) = 0;
    virtual void write(Float) = 0;
    virtual void write(Double) = 0;
    virtual void write(const ::std::string&, bool = true) = 0;
    virtual void write(const ::std::vector< ::std::string>&, bool) = 0; // Overload required for bool argument.
    virtual void write(const char*, bool = true) = 0;
    virtual void write(const ::std::wstring&) = 0;

    virtual void write(const bool*, const bool*) = 0;
    virtual void write(const Byte*, const Byte*) = 0;
    virtual void write(const Short*, const Short*) = 0;
    virtual void write(const Int*, const Int*) = 0;
    virtual void write(const Long*, const Long*) = 0;
    virtual void write(const Float*, const Float*) = 0;
    virtual void write(const Double*, const Double*) = 0;

    virtual void writeOptional(Int, OptionalType) = 0;

//
// COMPILER FIX: clang using libc++ cannot use the StreamHelper to write
// vector<bool>, as vector<bool> get optimized to
// __bit_const_reference that has not size member function.
//
#if defined(__clang__) && defined(_LIBCPP_VERSION)
    virtual void write(const ::std::vector<bool>& v)
    {
        writeSize(static_cast<Int>(v.size()));
        for(::std::vector<bool>::const_iterator p = v.begin(); p != v.end(); ++p)
        {
            bool v = (*p);
            write(v);
        }
    }
#endif

    template<typename T> inline void write(const T& v)
    {
        StreamHelper<T, StreamTrait<T>::type>::write(this, v);
    }

    template<typename T> inline void write(Ice::Int tag, const IceUtil::Optional<T>& v)
    {
        if(v)
        {
            writeOptional(tag, StreamOptionalHelper<T,
                                                    Ice::StreamTrait<T>::type, 
                                                    Ice::StreamTrait<T>::optionalType>::optionalType);
            StreamOptionalHelper<T, Ice::StreamTrait<T>::type, Ice::StreamTrait<T>::optionalType>::write(this, *v);
        }
    }

    //
    // Template functions for sequences and custom sequences
    // 
    template<typename T> void write(const T* begin, const T* end)
    {
        writeSize(static_cast<Ice::Int>(end - begin));
        for(const T* p = begin; p != end; ++p)
        {
            write(*p);
        }
    }
};

class ICE_API ObjectReader : public Object
{
public:

    virtual void read(const InputStreamPtr&) = 0;

private:

    virtual void __write(::IceInternal::BasicStream*) const;
    virtual void __read(::IceInternal::BasicStream*);

    virtual void __write(const OutputStreamPtr&) const;
    virtual void __read(const InputStreamPtr&);
};
typedef ::IceInternal::Handle<ObjectReader> ObjectReaderPtr;

class ICE_API ObjectWriter : public Object
{
public:

    virtual void write(const OutputStreamPtr&) const = 0;

private:

    virtual void __write(::IceInternal::BasicStream*) const;
    virtual void __read(::IceInternal::BasicStream*);

    virtual void __write(const OutputStreamPtr&) const;
    virtual void __read(const InputStreamPtr&);
};
typedef ::IceInternal::Handle<ObjectWriter> ObjectWriterPtr;

class ICE_API UserExceptionWriter : public UserException
{
public:

    UserExceptionWriter(const CommunicatorPtr&);
    ~UserExceptionWriter() throw();

    virtual void write(const OutputStreamPtr&) const = 0;
    virtual bool usesClasses() const = 0;

    virtual ::std::string ice_name() const = 0;
    virtual UserExceptionWriter* ice_clone() const = 0;
    virtual void ice_throw() const = 0;

    virtual void __write(IceInternal::BasicStream*) const;
    virtual void __read(IceInternal::BasicStream*);

    virtual bool __usesClasses() const;

#ifdef __SUNPRO_CC
    using UserException::__usesClasses;
#endif

protected:

    const CommunicatorPtr _communicator;
};

}

#endif
