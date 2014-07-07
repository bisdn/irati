// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: ApplicationProcessNamingInfoMessage.proto

#ifndef PROTOBUF_ApplicationProcessNamingInfoMessage_2eproto__INCLUDED
#define PROTOBUF_ApplicationProcessNamingInfoMessage_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2005000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2005000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

namespace rina {
namespace messages {

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_ApplicationProcessNamingInfoMessage_2eproto();
void protobuf_AssignDesc_ApplicationProcessNamingInfoMessage_2eproto();
void protobuf_ShutdownFile_ApplicationProcessNamingInfoMessage_2eproto();

class applicationProcessNamingInfo_t;

// ===================================================================

class applicationProcessNamingInfo_t : public ::google::protobuf::Message {
 public:
  applicationProcessNamingInfo_t();
  virtual ~applicationProcessNamingInfo_t();

  applicationProcessNamingInfo_t(const applicationProcessNamingInfo_t& from);

  inline applicationProcessNamingInfo_t& operator=(const applicationProcessNamingInfo_t& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const applicationProcessNamingInfo_t& default_instance();

  void Swap(applicationProcessNamingInfo_t* other);

  // implements Message ----------------------------------------------

  applicationProcessNamingInfo_t* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const applicationProcessNamingInfo_t& from);
  void MergeFrom(const applicationProcessNamingInfo_t& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required string applicationProcessName = 1;
  inline bool has_applicationprocessname() const;
  inline void clear_applicationprocessname();
  static const int kApplicationProcessNameFieldNumber = 1;
  inline const ::std::string& applicationprocessname() const;
  inline void set_applicationprocessname(const ::std::string& value);
  inline void set_applicationprocessname(const char* value);
  inline void set_applicationprocessname(const char* value, size_t size);
  inline ::std::string* mutable_applicationprocessname();
  inline ::std::string* release_applicationprocessname();
  inline void set_allocated_applicationprocessname(::std::string* applicationprocessname);

  // optional string applicationProcessInstance = 2;
  inline bool has_applicationprocessinstance() const;
  inline void clear_applicationprocessinstance();
  static const int kApplicationProcessInstanceFieldNumber = 2;
  inline const ::std::string& applicationprocessinstance() const;
  inline void set_applicationprocessinstance(const ::std::string& value);
  inline void set_applicationprocessinstance(const char* value);
  inline void set_applicationprocessinstance(const char* value, size_t size);
  inline ::std::string* mutable_applicationprocessinstance();
  inline ::std::string* release_applicationprocessinstance();
  inline void set_allocated_applicationprocessinstance(::std::string* applicationprocessinstance);

  // optional string applicationEntityName = 3;
  inline bool has_applicationentityname() const;
  inline void clear_applicationentityname();
  static const int kApplicationEntityNameFieldNumber = 3;
  inline const ::std::string& applicationentityname() const;
  inline void set_applicationentityname(const ::std::string& value);
  inline void set_applicationentityname(const char* value);
  inline void set_applicationentityname(const char* value, size_t size);
  inline ::std::string* mutable_applicationentityname();
  inline ::std::string* release_applicationentityname();
  inline void set_allocated_applicationentityname(::std::string* applicationentityname);

  // optional string applicationEntityInstance = 4;
  inline bool has_applicationentityinstance() const;
  inline void clear_applicationentityinstance();
  static const int kApplicationEntityInstanceFieldNumber = 4;
  inline const ::std::string& applicationentityinstance() const;
  inline void set_applicationentityinstance(const ::std::string& value);
  inline void set_applicationentityinstance(const char* value);
  inline void set_applicationentityinstance(const char* value, size_t size);
  inline ::std::string* mutable_applicationentityinstance();
  inline ::std::string* release_applicationentityinstance();
  inline void set_allocated_applicationentityinstance(::std::string* applicationentityinstance);

  // @@protoc_insertion_point(class_scope:rina.messages.applicationProcessNamingInfo_t)
 private:
  inline void set_has_applicationprocessname();
  inline void clear_has_applicationprocessname();
  inline void set_has_applicationprocessinstance();
  inline void clear_has_applicationprocessinstance();
  inline void set_has_applicationentityname();
  inline void clear_has_applicationentityname();
  inline void set_has_applicationentityinstance();
  inline void clear_has_applicationentityinstance();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::std::string* applicationprocessname_;
  ::std::string* applicationprocessinstance_;
  ::std::string* applicationentityname_;
  ::std::string* applicationentityinstance_;

  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(4 + 31) / 32];

  friend void  protobuf_AddDesc_ApplicationProcessNamingInfoMessage_2eproto();
  friend void protobuf_AssignDesc_ApplicationProcessNamingInfoMessage_2eproto();
  friend void protobuf_ShutdownFile_ApplicationProcessNamingInfoMessage_2eproto();

  void InitAsDefaultInstance();
  static applicationProcessNamingInfo_t* default_instance_;
};
// ===================================================================


// ===================================================================

// applicationProcessNamingInfo_t

// required string applicationProcessName = 1;
inline bool applicationProcessNamingInfo_t::has_applicationprocessname() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void applicationProcessNamingInfo_t::set_has_applicationprocessname() {
  _has_bits_[0] |= 0x00000001u;
}
inline void applicationProcessNamingInfo_t::clear_has_applicationprocessname() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void applicationProcessNamingInfo_t::clear_applicationprocessname() {
  if (applicationprocessname_ != &::google::protobuf::internal::kEmptyString) {
    applicationprocessname_->clear();
  }
  clear_has_applicationprocessname();
}
inline const ::std::string& applicationProcessNamingInfo_t::applicationprocessname() const {
  return *applicationprocessname_;
}
inline void applicationProcessNamingInfo_t::set_applicationprocessname(const ::std::string& value) {
  set_has_applicationprocessname();
  if (applicationprocessname_ == &::google::protobuf::internal::kEmptyString) {
    applicationprocessname_ = new ::std::string;
  }
  applicationprocessname_->assign(value);
}
inline void applicationProcessNamingInfo_t::set_applicationprocessname(const char* value) {
  set_has_applicationprocessname();
  if (applicationprocessname_ == &::google::protobuf::internal::kEmptyString) {
    applicationprocessname_ = new ::std::string;
  }
  applicationprocessname_->assign(value);
}
inline void applicationProcessNamingInfo_t::set_applicationprocessname(const char* value, size_t size) {
  set_has_applicationprocessname();
  if (applicationprocessname_ == &::google::protobuf::internal::kEmptyString) {
    applicationprocessname_ = new ::std::string;
  }
  applicationprocessname_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* applicationProcessNamingInfo_t::mutable_applicationprocessname() {
  set_has_applicationprocessname();
  if (applicationprocessname_ == &::google::protobuf::internal::kEmptyString) {
    applicationprocessname_ = new ::std::string;
  }
  return applicationprocessname_;
}
inline ::std::string* applicationProcessNamingInfo_t::release_applicationprocessname() {
  clear_has_applicationprocessname();
  if (applicationprocessname_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = applicationprocessname_;
    applicationprocessname_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}
inline void applicationProcessNamingInfo_t::set_allocated_applicationprocessname(::std::string* applicationprocessname) {
  if (applicationprocessname_ != &::google::protobuf::internal::kEmptyString) {
    delete applicationprocessname_;
  }
  if (applicationprocessname) {
    set_has_applicationprocessname();
    applicationprocessname_ = applicationprocessname;
  } else {
    clear_has_applicationprocessname();
    applicationprocessname_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  }
}

// optional string applicationProcessInstance = 2;
inline bool applicationProcessNamingInfo_t::has_applicationprocessinstance() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void applicationProcessNamingInfo_t::set_has_applicationprocessinstance() {
  _has_bits_[0] |= 0x00000002u;
}
inline void applicationProcessNamingInfo_t::clear_has_applicationprocessinstance() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void applicationProcessNamingInfo_t::clear_applicationprocessinstance() {
  if (applicationprocessinstance_ != &::google::protobuf::internal::kEmptyString) {
    applicationprocessinstance_->clear();
  }
  clear_has_applicationprocessinstance();
}
inline const ::std::string& applicationProcessNamingInfo_t::applicationprocessinstance() const {
  return *applicationprocessinstance_;
}
inline void applicationProcessNamingInfo_t::set_applicationprocessinstance(const ::std::string& value) {
  set_has_applicationprocessinstance();
  if (applicationprocessinstance_ == &::google::protobuf::internal::kEmptyString) {
    applicationprocessinstance_ = new ::std::string;
  }
  applicationprocessinstance_->assign(value);
}
inline void applicationProcessNamingInfo_t::set_applicationprocessinstance(const char* value) {
  set_has_applicationprocessinstance();
  if (applicationprocessinstance_ == &::google::protobuf::internal::kEmptyString) {
    applicationprocessinstance_ = new ::std::string;
  }
  applicationprocessinstance_->assign(value);
}
inline void applicationProcessNamingInfo_t::set_applicationprocessinstance(const char* value, size_t size) {
  set_has_applicationprocessinstance();
  if (applicationprocessinstance_ == &::google::protobuf::internal::kEmptyString) {
    applicationprocessinstance_ = new ::std::string;
  }
  applicationprocessinstance_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* applicationProcessNamingInfo_t::mutable_applicationprocessinstance() {
  set_has_applicationprocessinstance();
  if (applicationprocessinstance_ == &::google::protobuf::internal::kEmptyString) {
    applicationprocessinstance_ = new ::std::string;
  }
  return applicationprocessinstance_;
}
inline ::std::string* applicationProcessNamingInfo_t::release_applicationprocessinstance() {
  clear_has_applicationprocessinstance();
  if (applicationprocessinstance_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = applicationprocessinstance_;
    applicationprocessinstance_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}
inline void applicationProcessNamingInfo_t::set_allocated_applicationprocessinstance(::std::string* applicationprocessinstance) {
  if (applicationprocessinstance_ != &::google::protobuf::internal::kEmptyString) {
    delete applicationprocessinstance_;
  }
  if (applicationprocessinstance) {
    set_has_applicationprocessinstance();
    applicationprocessinstance_ = applicationprocessinstance;
  } else {
    clear_has_applicationprocessinstance();
    applicationprocessinstance_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  }
}

// optional string applicationEntityName = 3;
inline bool applicationProcessNamingInfo_t::has_applicationentityname() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void applicationProcessNamingInfo_t::set_has_applicationentityname() {
  _has_bits_[0] |= 0x00000004u;
}
inline void applicationProcessNamingInfo_t::clear_has_applicationentityname() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void applicationProcessNamingInfo_t::clear_applicationentityname() {
  if (applicationentityname_ != &::google::protobuf::internal::kEmptyString) {
    applicationentityname_->clear();
  }
  clear_has_applicationentityname();
}
inline const ::std::string& applicationProcessNamingInfo_t::applicationentityname() const {
  return *applicationentityname_;
}
inline void applicationProcessNamingInfo_t::set_applicationentityname(const ::std::string& value) {
  set_has_applicationentityname();
  if (applicationentityname_ == &::google::protobuf::internal::kEmptyString) {
    applicationentityname_ = new ::std::string;
  }
  applicationentityname_->assign(value);
}
inline void applicationProcessNamingInfo_t::set_applicationentityname(const char* value) {
  set_has_applicationentityname();
  if (applicationentityname_ == &::google::protobuf::internal::kEmptyString) {
    applicationentityname_ = new ::std::string;
  }
  applicationentityname_->assign(value);
}
inline void applicationProcessNamingInfo_t::set_applicationentityname(const char* value, size_t size) {
  set_has_applicationentityname();
  if (applicationentityname_ == &::google::protobuf::internal::kEmptyString) {
    applicationentityname_ = new ::std::string;
  }
  applicationentityname_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* applicationProcessNamingInfo_t::mutable_applicationentityname() {
  set_has_applicationentityname();
  if (applicationentityname_ == &::google::protobuf::internal::kEmptyString) {
    applicationentityname_ = new ::std::string;
  }
  return applicationentityname_;
}
inline ::std::string* applicationProcessNamingInfo_t::release_applicationentityname() {
  clear_has_applicationentityname();
  if (applicationentityname_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = applicationentityname_;
    applicationentityname_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}
inline void applicationProcessNamingInfo_t::set_allocated_applicationentityname(::std::string* applicationentityname) {
  if (applicationentityname_ != &::google::protobuf::internal::kEmptyString) {
    delete applicationentityname_;
  }
  if (applicationentityname) {
    set_has_applicationentityname();
    applicationentityname_ = applicationentityname;
  } else {
    clear_has_applicationentityname();
    applicationentityname_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  }
}

// optional string applicationEntityInstance = 4;
inline bool applicationProcessNamingInfo_t::has_applicationentityinstance() const {
  return (_has_bits_[0] & 0x00000008u) != 0;
}
inline void applicationProcessNamingInfo_t::set_has_applicationentityinstance() {
  _has_bits_[0] |= 0x00000008u;
}
inline void applicationProcessNamingInfo_t::clear_has_applicationentityinstance() {
  _has_bits_[0] &= ~0x00000008u;
}
inline void applicationProcessNamingInfo_t::clear_applicationentityinstance() {
  if (applicationentityinstance_ != &::google::protobuf::internal::kEmptyString) {
    applicationentityinstance_->clear();
  }
  clear_has_applicationentityinstance();
}
inline const ::std::string& applicationProcessNamingInfo_t::applicationentityinstance() const {
  return *applicationentityinstance_;
}
inline void applicationProcessNamingInfo_t::set_applicationentityinstance(const ::std::string& value) {
  set_has_applicationentityinstance();
  if (applicationentityinstance_ == &::google::protobuf::internal::kEmptyString) {
    applicationentityinstance_ = new ::std::string;
  }
  applicationentityinstance_->assign(value);
}
inline void applicationProcessNamingInfo_t::set_applicationentityinstance(const char* value) {
  set_has_applicationentityinstance();
  if (applicationentityinstance_ == &::google::protobuf::internal::kEmptyString) {
    applicationentityinstance_ = new ::std::string;
  }
  applicationentityinstance_->assign(value);
}
inline void applicationProcessNamingInfo_t::set_applicationentityinstance(const char* value, size_t size) {
  set_has_applicationentityinstance();
  if (applicationentityinstance_ == &::google::protobuf::internal::kEmptyString) {
    applicationentityinstance_ = new ::std::string;
  }
  applicationentityinstance_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* applicationProcessNamingInfo_t::mutable_applicationentityinstance() {
  set_has_applicationentityinstance();
  if (applicationentityinstance_ == &::google::protobuf::internal::kEmptyString) {
    applicationentityinstance_ = new ::std::string;
  }
  return applicationentityinstance_;
}
inline ::std::string* applicationProcessNamingInfo_t::release_applicationentityinstance() {
  clear_has_applicationentityinstance();
  if (applicationentityinstance_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = applicationentityinstance_;
    applicationentityinstance_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}
inline void applicationProcessNamingInfo_t::set_allocated_applicationentityinstance(::std::string* applicationentityinstance) {
  if (applicationentityinstance_ != &::google::protobuf::internal::kEmptyString) {
    delete applicationentityinstance_;
  }
  if (applicationentityinstance) {
    set_has_applicationentityinstance();
    applicationentityinstance_ = applicationentityinstance;
  } else {
    clear_has_applicationentityinstance();
    applicationentityinstance_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  }
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace messages
}  // namespace rina

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_ApplicationProcessNamingInfoMessage_2eproto__INCLUDED
