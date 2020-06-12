#ifndef SHADERINFO_H
#define SHADERINFO_H

#include "codec/samplebuffer.h"
#include "node/input.h"
#include "node/inputarray.h"
#include "node/value.h"

OLIVE_NAMESPACE_ENTER

using NodeValueMap = QHash<NodeInput*, NodeValue>;

class AcceleratedJob {
public:
  AcceleratedJob() = default;

  NodeValue GetValue(NodeInput* input) const
  {
    return value_map_.value(input);
  }

  void InsertValue(NodeInput* input, NodeValueDatabase& value)
  {
    if (input->IsArray()) {
      NodeInputArray* array = static_cast<NodeInputArray*>(input);
      QVector<NodeValue> values(array->GetSize());

      for (int j=0;j<array->GetSize();j++) {
        NodeInput* subparam = array->At(j);

        values[j] = value[subparam].TakeWithMeta(subparam->data_type());
      }

      value_map_.insert(input, NodeValue(NodeParam::kVec2, QVariant::fromValue(values), input->parentNode()));
    } else {
      value_map_.insert(input, value[input].TakeWithMeta(input->data_type()));
    }
  }

  void InsertValue(NodeInput* input, const NodeValue& value)
  {
    value_map_.insert(input, value);
  }

  const NodeValueMap &GetValues() const
  {
    return value_map_;
  }

private:
  NodeValueMap value_map_;

};

class SampleJob : public AcceleratedJob {
public:
  SampleJob()
  {
    samples_ = nullptr;
  }

  SampleJob(const NodeValue& value)
  {
    samples_ = value.data().value<SampleBufferPtr>();
  }

  SampleJob(NodeInput* from, NodeValueDatabase& db)
  {
    samples_ = db[from].Take(NodeParam::kSamples).value<SampleBufferPtr>();
  }

  SampleBufferPtr samples() const
  {
    return samples_;
  }

  bool HasSamples() const
  {
    return samples_ && samples_->is_allocated();
  }

private:
  SampleBufferPtr samples_;

};

class ShaderJob : public AcceleratedJob {
public:
  ShaderJob()
  {
    iterations_ = 1;
    iterative_input_ = nullptr;
    alpha_channel_required_ = false;
  }

  const QString& GetShaderID() const
  {
    return id_;
  }

  void SetShaderID(const QString& id)
  {
    id_ = id;
  }

  void SetIterations(int iterations, NodeInput* iterative_input)
  {
    iterations_ = iterations;
    iterative_input_ = iterative_input;
  }

  int GetIterationCount() const
  {
    return iterations_;
  }

  NodeInput* GetIterativeInput() const
  {
    return iterative_input_;
  }

  bool GetAlphaChannelRequired() const
  {
    return alpha_channel_required_;
  }

  void SetAlphaChannelRequired(bool e)
  {
    alpha_channel_required_ = e;
  }

private:
  QString id_;

  int iterations_;

  NodeInput* iterative_input_;

  bool alpha_channel_required_;

};

class ShaderCode {
public:
  ShaderCode(const QString& frag_code, const QString& vert_code) :
    frag_code_(frag_code),
    vert_code_(vert_code)
  {
  }

  const QString& frag_code() const
  {
    return frag_code_;
  }

  const QString& vert_code() const
  {
    return vert_code_;
  }

private:
  QString frag_code_;

  QString vert_code_;

};

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::ShaderJob)
Q_DECLARE_METATYPE(OLIVE_NAMESPACE::SampleJob)

#endif // SHADERINFO_H
