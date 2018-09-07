//
// Header-only tiny glTF 2.0 loader and serializer.
//
//
// The MIT License (MIT)
//
// Copyright (c) 2015 - 2017 Syoyo Fujita, Aurélien Chatelain and many
// contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// Version:
//  - v2.0.0 glTF 2.0!.
//
// Tiny glTF loader is using following third party libraries:
//
//  - jsonhpp: C++ JSON library.
//  - base64: base64 decode/encode library.
//  - stb_image: Image loading library.
//
#ifndef TINY_GLTF_H_
#define TINY_GLTF_H_

#include <cstdint>
#include <cassert>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <array>

namespace tinygltf {

#define TINYGLTF_MODE_POINTS (0)
#define TINYGLTF_MODE_LINE (1)
#define TINYGLTF_MODE_LINE_LOOP (2)
#define TINYGLTF_MODE_TRIANGLES (4)
#define TINYGLTF_MODE_TRIANGLE_STRIP (5)
#define TINYGLTF_MODE_TRIANGLE_FAN (6)

#define TINYGLTF_COMPONENT_TYPE_BYTE (5120)
#define TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE (5121)
#define TINYGLTF_COMPONENT_TYPE_SHORT (5122)
#define TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT (5123)
#define TINYGLTF_COMPONENT_TYPE_INT (5124)
#define TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT (5125)
#define TINYGLTF_COMPONENT_TYPE_FLOAT (5126)
#define TINYGLTF_COMPONENT_TYPE_DOUBLE (5130)

#define TINYGLTF_TEXTURE_FILTER_NEAREST (9728)
#define TINYGLTF_TEXTURE_FILTER_LINEAR (9729)
#define TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST (9984)
#define TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST (9985)
#define TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR (9986)
#define TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR (9987)

#define TINYGLTF_TEXTURE_WRAP_REPEAT (10497)
#define TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE (33071)
#define TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT (33648)

// Redeclarations of the above for technique.parameters.
#define TINYGLTF_PARAMETER_TYPE_BYTE (5120)
#define TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE (5121)
#define TINYGLTF_PARAMETER_TYPE_SHORT (5122)
#define TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT (5123)
#define TINYGLTF_PARAMETER_TYPE_INT (5124)
#define TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT (5125)
#define TINYGLTF_PARAMETER_TYPE_FLOAT (5126)

#define TINYGLTF_PARAMETER_TYPE_FLOAT_VEC2 (35664)
#define TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 (35665)
#define TINYGLTF_PARAMETER_TYPE_FLOAT_VEC4 (35666)

#define TINYGLTF_PARAMETER_TYPE_INT_VEC2 (35667)
#define TINYGLTF_PARAMETER_TYPE_INT_VEC3 (35668)
#define TINYGLTF_PARAMETER_TYPE_INT_VEC4 (35669)

#define TINYGLTF_PARAMETER_TYPE_BOOL (35670)
#define TINYGLTF_PARAMETER_TYPE_BOOL_VEC2 (35671)
#define TINYGLTF_PARAMETER_TYPE_BOOL_VEC3 (35672)
#define TINYGLTF_PARAMETER_TYPE_BOOL_VEC4 (35673)

#define TINYGLTF_PARAMETER_TYPE_FLOAT_MAT2 (35674)
#define TINYGLTF_PARAMETER_TYPE_FLOAT_MAT3 (35675)
#define TINYGLTF_PARAMETER_TYPE_FLOAT_MAT4 (35676)

#define TINYGLTF_PARAMETER_TYPE_SAMPLER_2D (35678)

// End parameter types

#define TINYGLTF_TYPE_VEC2 (2)
#define TINYGLTF_TYPE_VEC3 (3)
#define TINYGLTF_TYPE_VEC4 (4)
#define TINYGLTF_TYPE_MAT2 (32 + 2)
#define TINYGLTF_TYPE_MAT3 (32 + 3)
#define TINYGLTF_TYPE_MAT4 (32 + 4)
#define TINYGLTF_TYPE_SCALAR (64 + 1)
#define TINYGLTF_TYPE_VECTOR (64 + 4)
#define TINYGLTF_TYPE_MATRIX (64 + 16)

#define TINYGLTF_IMAGE_FORMAT_JPEG (0)
#define TINYGLTF_IMAGE_FORMAT_PNG (1)
#define TINYGLTF_IMAGE_FORMAT_BMP (2)
#define TINYGLTF_IMAGE_FORMAT_GIF (3)

#define TINYGLTF_TEXTURE_FORMAT_ALPHA (6406)
#define TINYGLTF_TEXTURE_FORMAT_RGB (6407)
#define TINYGLTF_TEXTURE_FORMAT_RGBA (6408)
#define TINYGLTF_TEXTURE_FORMAT_LUMINANCE (6409)
#define TINYGLTF_TEXTURE_FORMAT_LUMINANCE_ALPHA (6410)

#define TINYGLTF_TEXTURE_TARGET_TEXTURE2D (3553)
#define TINYGLTF_TEXTURE_TYPE_UNSIGNED_BYTE (5121)

#define TINYGLTF_TARGET_ARRAY_BUFFER (34962)
#define TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER (34963)

#define TINYGLTF_SHADER_TYPE_VERTEX_SHADER (35633)
#define TINYGLTF_SHADER_TYPE_FRAGMENT_SHADER (35632)

typedef enum {
  NULL_TYPE = 0,
  NUMBER_TYPE = 1,
  INT_TYPE = 2,
  BOOL_TYPE = 3,
  STRING_TYPE = 4,
  ARRAY_TYPE = 5,
  BINARY_TYPE = 6,
  OBJECT_TYPE = 7
} Type;

static inline int32_t GetComponentSizeInBytes(uint32_t componentType)
{
  if (componentType == TINYGLTF_COMPONENT_TYPE_BYTE) {
    return 1;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
    return 1;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_SHORT) {
    return 2;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
    return 2;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_INT) {
    return 4;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
    return 4;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
    return 4;
  } else if (componentType == TINYGLTF_COMPONENT_TYPE_DOUBLE) {
    return 8;
  } else {
    // Unknown componenty type
    return -1;
  }
}

static inline int32_t GetTypeSizeInBytes(uint32_t ty)
{
  if (ty == TINYGLTF_TYPE_SCALAR) {
    return 1;
  } else if (ty == TINYGLTF_TYPE_VEC2) {
    return 2;
  } else if (ty == TINYGLTF_TYPE_VEC3) {
    return 3;
  } else if (ty == TINYGLTF_TYPE_VEC4) {
    return 4;
  } else if (ty == TINYGLTF_TYPE_MAT2) {
    return 4;
  } else if (ty == TINYGLTF_TYPE_MAT3) {
    return 9;
  } else if (ty == TINYGLTF_TYPE_MAT4) {
    return 16;
  } else {
    // Unknown componenty type
    return -1;
  }
}

#ifdef __clang__
#pragma clang diagnostic push
// Suppress warning for : static Value null_value
// https://stackoverflow.com/questions/15708411/how-to-deal-with-global-constructor-warning-in-clang
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wpadded"
#endif

// Simple class to represent JSON object
class Value {
 public:
  typedef std::vector<Value> Array;
  typedef std::map<std::string, Value> Object;

  Value() : type_(NULL_TYPE) {}

  explicit Value(bool b) : type_(BOOL_TYPE) { boolean_value_ = b; }
  explicit Value(int i) : type_(INT_TYPE) { int_value_ = i; }
  explicit Value(double n) : type_(NUMBER_TYPE) { number_value_ = n; }
  explicit Value(const std::string &s) : type_(STRING_TYPE) {
    string_value_ = s;
  }
  explicit Value(const unsigned char *p, size_t n) : type_(BINARY_TYPE) {
    binary_value_.resize(n);
    memcpy(binary_value_.data(), p, n);
  }
  explicit Value(const Array &a) : type_(ARRAY_TYPE) {
    array_value_ = Array(a);
  }
  explicit Value(const Object &o) : type_(OBJECT_TYPE) {
    object_value_ = Object(o);
  }

  char Type() const { return static_cast<const char>(type_); }

  bool IsBool() const { return (type_ == BOOL_TYPE); }

  bool IsInt() const { return (type_ == INT_TYPE); }

  bool IsNumber() const { return (type_ == NUMBER_TYPE); }

  bool IsString() const { return (type_ == STRING_TYPE); }

  bool IsBinary() const { return (type_ == BINARY_TYPE); }

  bool IsArray() const { return (type_ == ARRAY_TYPE); }

  bool IsObject() const { return (type_ == OBJECT_TYPE); }

  // Accessor
  template <typename T>
  const T &Get() const;
  template <typename T>
  T &Get();

  // Lookup value from an array
  const Value &Get(int idx) const {
    static Value null_value;
    assert(IsArray());
    assert(idx >= 0);
    return (static_cast<size_t>(idx) < array_value_.size())
               ? array_value_[static_cast<size_t>(idx)]
               : null_value;
  }

  // Lookup value from a key-value pair
  const Value &Get(const std::string &key) const {
    static Value null_value;
    assert(IsObject());
    Object::const_iterator it = object_value_.find(key);
    return (it != object_value_.end()) ? it->second : null_value;
  }

  size_t ArrayLen() const {
    if (!IsArray()) return 0;
    return array_value_.size();
  }

  // Valid only for object type.
  bool Has(const std::string &key) const {
    if (!IsObject()) return false;
    Object::const_iterator it = object_value_.find(key);
    return (it != object_value_.end()) ? true : false;
  }

  // List keys
  std::vector<std::string> Keys() const {
    std::vector<std::string> keys;
    if (!IsObject()) return keys;  // empty

    for (Object::const_iterator it = object_value_.begin();
         it != object_value_.end(); ++it) {
      keys.push_back(it->first);
    }

    return keys;
  }

  size_t Size() const { return (IsArray() ? ArrayLen() : Keys().size()); }

 protected:
  int type_;

  int int_value_;
  double number_value_;
  std::string string_value_;
  std::vector<unsigned char> binary_value_;
  Array array_value_;
  Object object_value_;
  bool boolean_value_;
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif


#define TINYGLTF_VALUE_GET(ctype, var)            \
  template <>                                     \
  inline const ctype &Value::Get<ctype>() const { \
    return var;                                   \
  }                                               \
  template <>                                     \
  inline ctype &Value::Get<ctype>() {             \
    return var;                                   \
  }
TINYGLTF_VALUE_GET(bool, boolean_value_)
TINYGLTF_VALUE_GET(double, number_value_)
TINYGLTF_VALUE_GET(int, int_value_)
TINYGLTF_VALUE_GET(std::string, string_value_)
TINYGLTF_VALUE_GET(std::vector<unsigned char>, binary_value_)
TINYGLTF_VALUE_GET(Value::Array, array_value_)
TINYGLTF_VALUE_GET(Value::Object, object_value_)
#undef TINYGLTF_VALUE_GET

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat"
#pragma clang diagnostic ignored "-Wpadded"
#endif

///Agregate object for representing a color
using ColorValue = std::array<double, 4>;

 struct Parameter {
  bool bool_value;
  std::string string_value;
  std::vector<double> number_array;
  std::map<std::string, double> json_double_value;

  //context sensitive methods. depending the type of the Parameter you are accessing, these are either valid or not
  //If this parameter represent a texture map in a material, will return the texture index

  ///Return the index of a texture if this Parameter is a texture map.
  ///Returned value is only valid if the parameter represent a texture from a material
  int TextureIndex() const {
    const auto it = json_double_value.find("index");
    if (it != std::end(json_double_value))
    {
      return int(it->second);
    }
    return -1;
  }

  ///Material factor, like the roughness or metalness of a material
  ///Returned value is only valid if the parameter represent a texture from a material
  double Factor() const {
    return number_array[0];
  }

  ///Return the color of a material
  ///Returned value is only valid if the parameter represent a texture from a material
  ColorValue ColorFactor() const {
    return {
      { // this agregate intialize the std::array object, and uses C++11 RVO.
        number_array[0],
        number_array[1],
        number_array[2],
        (number_array.size() > 3 ? number_array[3] : 1.0)
      }
    };
  }
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif

typedef std::map<std::string, Parameter> ParameterMap;

struct AnimationChannel {
  int sampler;              // required
  int target_node;          // required (index of the node to target)
  std::string target_path;  // required in ["translation", "rotation", "scale",
                            // "weights"]
  Value extras;

  AnimationChannel() : sampler(-1), target_node(-1) {}
};

struct AnimationSampler {
  int input;                  // required
  int output;                 // required
  std::string interpolation;  // in ["LINEAR", "STEP", "CATMULLROMSPLINE",
                              // "CUBICSPLINE"], default "LINEAR"

  AnimationSampler() : input(-1), output(-1), interpolation("LINEAR") {}
};

typedef struct {
  std::string name;
  std::vector<AnimationChannel> channels;
  std::vector<AnimationSampler> samplers;
  Value extras;
} Animation;

struct Skin {
  std::string name;
  int inverseBindMatrices;  // required here but not in the spec
  int skeleton;             // The index of the node used as a skeleton root
  std::vector<int> joints;  // Indices of skeleton nodes

  Skin() {
    inverseBindMatrices = -1;
    skeleton = -1;
  }
};

struct Sampler {
  std::string name;
  int minFilter;  // ["NEAREST", "LINEAR", "NEAREST_MIPMAP_LINEAR",
                  // "LINEAR_MIPMAP_NEAREST", "NEAREST_MIPMAP_LINEAR",
                  // "LINEAR_MIPMAP_LINEAR"]
  int magFilter;  // ["NEAREST", "LINEAR"]
  int wrapS;      // ["CLAMP_TO_EDGE", "MIRRORED_REPEAT", "REPEAT"], default
                  // "REPEAT"
  int wrapT;      // ["CLAMP_TO_EDGE", "MIRRORED_REPEAT", "REPEAT"], default
                  // "REPEAT"
  int wrapR;      // TinyGLTF extension
  Value extras;

  Sampler()
      : wrapS(TINYGLTF_TEXTURE_WRAP_REPEAT),
        wrapT(TINYGLTF_TEXTURE_WRAP_REPEAT) {}
};

struct Image {
  std::string name;
  int width;
  int height;
  int component;
  std::vector<unsigned char> image;
  int bufferView;        // (required if no uri)
  std::string mimeType;  // (required if no uri) ["image/jpeg", "image/png"]
  std::string uri;       // (reqiored if no mimeType)
  Value extras;

  Image() { bufferView = -1; }
};

struct Texture {
  int sampler;
  int source;  // Required (not specified in the spec ?)
  Value extras;

  Texture() : sampler(-1), source(-1) {}
};

// Each extension should be stored in a ParameterMap.
// members not in the values could be included in the ParameterMap
// to keep a single material model
struct Material {
  std::string name;

  ParameterMap values;            // PBR metal/roughness workflow
  ParameterMap additionalValues;  // normal/occlusion/emissive values
  ParameterMap extCommonValues;   // KHR_common_material extension
  ParameterMap extPBRValues;
  Value extras;
};

struct BufferView {
  std::string name;
  int buffer;         // Required
  size_t byteOffset;  // minimum 0, default 0
  size_t byteLength;  // required, minimum 1
  size_t byteStride;  // minimum 4, maximum 252 (multiple of 4), default 0 =
                      // understood to be tightly packed
  int target;         // ["ARRAY_BUFFER", "ELEMENT_ARRAY_BUFFER"]
  Value extras;

  BufferView() : byteOffset(0), byteStride(0) {}
};

struct Accessor {
  int bufferView;  // optional in spec but required here since sparse accessor
                   // are not supported
  std::string name;
  size_t byteOffset;
  bool normalized;    // optinal.
  int componentType;  // (required) One of TINYGLTF_COMPONENT_TYPE_***
  size_t count;       // required
  int type;           // (required) One of TINYGLTF_TYPE_***   ..
  Value extras;

  std::vector<double> minValues;  // optional
  std::vector<double> maxValues;  // optional

  // TODO(syoyo): "sparse"

  ///
  /// Utility function to compute byteStride for a given bufferView object.
  /// Returns -1 upon invalid glTF value or parameter configuration.
  ///
  int ByteStride(const BufferView &bufferViewObject) const {
    if (bufferViewObject.byteStride == 0) {
      // Assume data is tightly packed.
      int componentSizeInBytes = GetComponentSizeInBytes(static_cast<uint32_t>(componentType));
      if (componentSizeInBytes <= 0) {
        return -1;
      }

      int typeSizeInBytes = GetTypeSizeInBytes(static_cast<uint32_t>(type));
      if (typeSizeInBytes <= 0) {
        return -1;
      }

      return componentSizeInBytes * typeSizeInBytes;
    } else {
      // Check if byteStride is a mulple of the size of the accessor's component type.
      int componentSizeInBytes = GetComponentSizeInBytes(static_cast<uint32_t>(componentType));
      if (componentSizeInBytes <= 0) {
        return -1;
      }

      if ((bufferViewObject.byteStride % uint32_t(componentSizeInBytes)) != 0) {
        return -1;
      }
      return static_cast<int>(bufferViewObject.byteStride);
    }

    return 0;
  }

  Accessor() { bufferView = -1; }
};

struct PerspectiveCamera {
  float aspectRatio;  // min > 0
  float yfov;         // required. min > 0
  float zfar;         // min > 0
  float znear;        // required. min > 0

  PerspectiveCamera()
      : aspectRatio(0.0f),
        yfov(0.0f),
        zfar(0.0f)  // 0 = use infinite projecton matrix
        ,
        znear(0.0f) {}

  ParameterMap extensions;
  Value extras;
};

struct OrthographicCamera {
  float xmag;   // required. must not be zero.
  float ymag;   // required. must not be zero.
  float zfar;   // required. `zfar` must be greater than `znear`.
  float znear;  // required

  OrthographicCamera() : xmag(0.0f), ymag(0.0f), zfar(0.0f), znear(0.0f) {}

  ParameterMap extensions;
  Value extras;
};

struct Camera {
  std::string type;  // required. "perspective" or "orthographic"
  std::string name;

  PerspectiveCamera perspective;
  OrthographicCamera orthographic;

  Camera() {}

  ParameterMap extensions;
  Value extras;
};

struct Primitive {
  std::map<std::string, int> attributes;  // (required) A dictionary object of
                                          // integer, where each integer
                                          // is the index of the accessor
                                          // containing an attribute.
  int material;  // The index of the material to apply to this primitive
                 // when rendering.
  int indices;   // The index of the accessor that contains the indices.
  int mode;      // one of TINYGLTF_MODE_***
  std::vector<std::map<std::string, int> > targets;  // array of morph targets,
  // where each target is a dict with attribues in ["POSITION, "NORMAL",
  // "TANGENT"] pointing
  // to their corresponding accessors
  Value extras;

  Primitive() {
    material = -1;
    indices = -1;
  }
};

typedef struct {
  std::string name;
  std::vector<Primitive> primitives;
  std::vector<double> weights;  // weights to be applied to the Morph Targets
  std::vector<std::map<std::string, int> > targets;
  ParameterMap extensions;
  Value extras;
} Mesh;

class Node {
 public:
  Node() : camera(-1), skin(-1), mesh(-1) {}

  Node(const Node &rhs) {
    camera = rhs.camera;

    name = rhs.name;
    skin = rhs.skin;
    mesh = rhs.mesh;
    children = rhs.children;
    rotation = rhs.rotation;
    scale = rhs.scale;
    translation = rhs.translation;
    matrix = rhs.matrix;
    weights = rhs.weights;

    extras = rhs.extras;
    extLightsValues = rhs.extLightsValues;
  }

  ~Node() {}

  int camera;  // the index of the camera referenced by this node

  std::string name;
  int skin;
  int mesh;
  std::vector<int> children;
  std::vector<double> rotation;     // length must be 0 or 4
  std::vector<double> scale;        // length must be 0 or 3
  std::vector<double> translation;  // length must be 0 or 3
  std::vector<double> matrix;       // length must be 0 or 16
  std::vector<double> weights;  // The weights of the instantiated Morph Target

  Value extras;
  ParameterMap extLightsValues;      // KHR_lights_cmn extension
};

typedef struct {
  std::string name;
  std::vector<unsigned char> data;
  std::string
      uri;  // considered as required here but not in the spec (need to clarify)
  Value extras;
} Buffer;

typedef struct {
  std::string version;  // required
  std::string generator;
  std::string minVersion;
  std::string copyright;
  ParameterMap extensions;
  Value extras;
} Asset;

struct Scene {
  std::string name;
  std::vector<int> nodes;

  ParameterMap extensions;
  ParameterMap extras;
};

struct Light {
  std::string name;
  std::vector<double> color;
  std::string type;
};

class Model {
 public:
  Model() {}
  ~Model() {}

  std::vector<Accessor> accessors;
  std::vector<Animation> animations;
  std::vector<Buffer> buffers;
  std::vector<BufferView> bufferViews;
  std::vector<Material> materials;
  std::vector<Mesh> meshes;
  std::vector<Node> nodes;
  std::vector<Texture> textures;
  std::vector<Image> images;
  std::vector<Skin> skins;
  std::vector<Sampler> samplers;
  std::vector<Camera> cameras;
  std::vector<Scene> scenes;
  std::vector<Light> lights;

  int defaultScene;
  std::vector<std::string> extensionsUsed;
  std::vector<std::string> extensionsRequired;

  Asset asset;

  Value extras;
};

enum SectionCheck {
  NO_REQUIRE = 0x00,
  REQUIRE_SCENE = 0x01,
  REQUIRE_SCENES = 0x02,
  REQUIRE_NODES = 0x04,
  REQUIRE_ACCESSORS = 0x08,
  REQUIRE_BUFFERS = 0x10,
  REQUIRE_BUFFER_VIEWS = 0x20,
  REQUIRE_ALL = 0x3f
};


class TinyGLTF {
 public:

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat"
#endif

  TinyGLTF() : bin_data_(nullptr), bin_size_(0), is_binary_(false) {
  }

#ifdef __clang__
#pragma clang diagnostic pop
#endif

  ~TinyGLTF() {}

  ///
  /// Loads glTF ASCII asset from a file.
  /// Returns false and set error string to `err` if there's an error.
  ///
  bool LoadASCIIFromFile(Model *model, std::string *err,
                         const std::string &filename,
                         unsigned int check_sections = REQUIRE_ALL);

  ///
  /// Loads glTF ASCII asset from string(memory).
  /// `length` = strlen(str);
  /// Returns false and set error string to `err` if there's an error.
  ///
  bool LoadASCIIFromString(Model *model, std::string *err, const char *str,
                           const unsigned int length,
                           const std::string &base_dir,
                           unsigned int check_sections = REQUIRE_ALL);

  ///
  /// Loads glTF binary asset from a file.
  /// Returns false and set error string to `err` if there's an error.
  ///
  bool LoadBinaryFromFile(Model *model, std::string *err,
                          const std::string &filename,
                          unsigned int check_sections = REQUIRE_ALL);

  ///
  /// Loads glTF binary asset from memory.
  /// `length` = strlen(str);
  /// Returns false and set error string to `err` if there's an error.
  ///
  bool LoadBinaryFromMemory(Model *model, std::string *err,
                            const unsigned char *bytes,
                            const unsigned int length,
                            const std::string &base_dir = "",
                            unsigned int check_sections = REQUIRE_ALL);

  ///
  /// Write glTF to file.
  ///
  bool WriteGltfSceneToFile(
      Model *model,
      const std::string &
          filename /*, bool embedImages, bool embedBuffers, bool writeBinary*/);

 private:
  ///
  /// Loads glTF asset from string(memory).
  /// `length` = strlen(str);
  /// Returns false and set error string to `err` if there's an error.
  ///
  bool LoadFromString(Model *model, std::string *err, const char *str,
                      const unsigned int length, const std::string &base_dir,
                      unsigned int check_sections);

  const unsigned char *bin_data_;
  size_t bin_size_;
  bool is_binary_;
};

#ifdef __clang__
#pragma clang diagnostic pop // -Wpadded
#endif

}  // namespace tinygltf

#endif  // TINY_GLTF_H_

#ifdef TINYGLTF_IMPLEMENTATION
#include <algorithm>
//#include <cassert>
#include <fstream>
#include <sstream>

#ifdef __clang__
// Disable some warnings for external files.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wc++98-compat"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#if __has_warning("-Wcomma")
#pragma clang diagnostic ignored "-Wcomma"
#endif
#if __has_warning("-Wzero-as-null-pointer-constant")
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#if __has_warning("-Wcast-qual")
#pragma clang diagnostic ignored "-Wcast-qual"
#endif
#endif

#include "./json.hpp"
#include "./stb_image.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef _WIN32
#include <windows.h>
#elif !defined(__ANDROID__)
#include <wordexp.h>
#endif

#if defined(__sparcv9)
// Big endian
#else
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || MINIZ_X86_OR_X64_CPU
#define TINYGLTF_LITTLE_ENDIAN 1
#endif
#endif

using nlohmann::json;

#ifdef __APPLE__
    #include "TargetConditionals.h"
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat"
#endif

namespace tinygltf {

static void swap4(unsigned int *val) {
#ifdef TINYGLTF_LITTLE_ENDIAN
  (void)val;
#else
  unsigned int tmp = *val;
  unsigned char *dst = reinterpret_cast<unsigned char *>(val);
  unsigned char *src = reinterpret_cast<unsigned char *>(&tmp);

  dst[0] = src[3];
  dst[1] = src[2];
  dst[2] = src[1];
  dst[3] = src[0];
#endif
}

static bool FileExists(const std::string &abs_filename) {
  bool ret;
#ifdef _WIN32
  FILE *fp;
  errno_t err = fopen_s(&fp, abs_filename.c_str(), "rb");
  if (err != 0) {
    return false;
  }
#else
  FILE *fp = fopen(abs_filename.c_str(), "rb");
#endif
  if (fp) {
    ret = true;
    fclose(fp);
  } else {
    ret = false;
  }

  return ret;
}

static std::string ExpandFilePath(const std::string &filepath) {
#ifdef _WIN32
  DWORD len = ExpandEnvironmentStringsA(filepath.c_str(), NULL, 0);
  char *str = new char[len];
  ExpandEnvironmentStringsA(filepath.c_str(), str, len);

  std::string s(str);

  delete[] str;

  return s;
#else

#if defined(TARGET_OS_IPHONE) || defined(TARGET_IPHONE_SIMULATOR) || defined(__ANDROID__)
  // no expansion
  std::string s = filepath;
#else
  std::string s;
  wordexp_t p;

  if (filepath.empty()) {
    return "";
  }

  // char** w;
  int ret = wordexp(filepath.c_str(), &p, 0);
  if (ret) {
    // err
    s = filepath;
    return s;
  }

  // Use first element only.
  if (p.we_wordv) {
    s = std::string(p.we_wordv[0]);
    wordfree(&p);
  } else {
    s = filepath;
  }

#endif

  return s;
#endif
}

static std::string JoinPath(const std::string &path0,
                            const std::string &path1) {
  if (path0.empty()) {
    return path1;
  } else {
    // check '/'
    char lastChar = *path0.rbegin();
    if (lastChar != '/') {
      return path0 + std::string("/") + path1;
    } else {
      return path0 + path1;
    }
  }
}

static std::string FindFile(const std::vector<std::string> &paths,
                            const std::string &filepath) {
  for (size_t i = 0; i < paths.size(); i++) {
    std::string absPath = ExpandFilePath(JoinPath(paths[i], filepath));
    if (FileExists(absPath)) {
      return absPath;
    }
  }

  return std::string();
}

// std::string GetFilePathExtension(const std::string& FileName)
//{
//    if(FileName.find_last_of(".") != std::string::npos)
//        return FileName.substr(FileName.find_last_of(".")+1);
//    return "";
//}

static std::string GetBaseDir(const std::string &filepath) {
  if (filepath.find_last_of("/\\") != std::string::npos)
    return filepath.substr(0, filepath.find_last_of("/\\"));
  return "";
}

// std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const &s);

/*
   base64.cpp and base64.h

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wconversion"
#endif
static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_decode(std::string const &encoded_string) {
  int in_len = static_cast<int>(encoded_string.size());
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && (encoded_string[in_] != '=') &&
         is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_];
    in_++;
    if (i == 4) {
      for (i = 0; i < 4; i++)
        char_array_4[i] =
            static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

      char_array_3[0] =
          (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] =
          ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++) ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 4; j++) char_array_4[j] = 0;

    for (j = 0; j < 4; j++)
      char_array_4[j] =
          static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] =
        ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif

static bool LoadExternalFile(std::vector<unsigned char> *out, std::string *err,
                             const std::string &filename,
                             const std::string &basedir, size_t reqBytes,
                             bool checkSize) {
  out->clear();

  std::vector<std::string> paths;
  paths.push_back(basedir);
  paths.push_back(".");

  std::string filepath = FindFile(paths, filename);
  if (filepath.empty() || filename.empty()) {
    if (err) {
      (*err) += "File not found : " + filename + "\n";
    }
    return false;
  }

  std::ifstream f(filepath.c_str(), std::ifstream::binary);
  if (!f) {
    if (err) {
      (*err) += "File open error : " + filepath + "\n";
    }
    return false;
  }

  f.seekg(0, f.end);
  size_t sz = static_cast<size_t>(f.tellg());
  if (int(sz) < 0) {
    // Looks reading directory, not a file.
    return false;
  }

  if (sz == 0) {
    // Invalid file size.
    return false;
  }
  std::vector<unsigned char> buf(sz);

  f.seekg(0, f.beg);
  f.read(reinterpret_cast<char *>(&buf.at(0)),
         static_cast<std::streamsize>(sz));
  f.close();

  if (checkSize) {
    if (reqBytes == sz) {
      out->swap(buf);
      return true;
    } else {
      std::stringstream ss;
      ss << "File size mismatch : " << filepath << ", requestedBytes "
         << reqBytes << ", but got " << sz << std::endl;
      if (err) {
        (*err) += ss.str();
      }
      return false;
    }
  }

  out->swap(buf);
  return true;
}

static bool LoadImageData(Image *image, std::string *err, int req_width,
                          int req_height, const unsigned char *bytes,
                          int size) {
  //std::cout << "size " << size << std::endl;

  int w, h, comp;
  // if image cannot be decoded, ignore parsing and keep it by its path
  // don't break in this case
  // FIXME we should only enter this function if the image is embedded. If
  // image->uri references
  // an image file, it should be left as it is. Image loading should not be
  // mandatory (to support other formats)
  unsigned char *data = stbi_load_from_memory(bytes, size, &w, &h, &comp, 0);
  if (!data) {
    if (err) {
      (*err) += "Unknown image format.\n";
    }
    return true;
  }

  if (w < 1 || h < 1) {
    free(data);
    if (err) {
      (*err) += "Invalid image data.\n";
    }
    return true;
  }

  if (req_width > 0) {
    if (req_width != w) {
      free(data);
      if (err) {
        (*err) += "Image width mismatch.\n";
      }
      return false;
    }
  }

  if (req_height > 0) {
    if (req_height != h) {
      free(data);
      if (err) {
        (*err) += "Image height mismatch.\n";
      }
      return false;
    }
  }

  image->width = w;
  image->height = h;
  image->component = comp;
  image->image.resize(static_cast<size_t>(w * h * comp));
  std::copy(data, data + w * h * comp, image->image.begin());

  free(data);

  return true;
}

static bool IsDataURI(const std::string &in) {
  std::string header = "data:application/octet-stream;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:image/png;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:image/jpeg;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  header = "data:text/plain;base64,";
  if (in.find(header) == 0) {
    return true;
  }

  return false;
}

static bool DecodeDataURI(std::vector<unsigned char> *out,
                          const std::string &in, size_t reqBytes,
                          bool checkSize) {
  std::string header = "data:application/octet-stream;base64,";
  std::string data;
  if (in.find(header) == 0) {
    data = base64_decode(in.substr(header.size()));  // cut mime string.
  }

  if (data.empty()) {
    header = "data:image/jpeg;base64,";
    if (in.find(header) == 0) {
      data = base64_decode(in.substr(header.size()));  // cut mime string.
    }
  }

  if (data.empty()) {
    header = "data:image/png;base64,";
    if (in.find(header) == 0) {
      data = base64_decode(in.substr(header.size()));  // cut mime string.
    }
  }

  if (data.empty()) {
    header = "data:text/plain;base64,";
    if (in.find(header) == 0) {
      data = base64_decode(in.substr(header.size()));
    }
  }

  if (data.empty()) {
    return false;
  }

  if (checkSize) {
    if (data.size() != reqBytes) {
      return false;
    }
    out->resize(reqBytes);
  } else {
    out->resize(data.size());
  }
  std::copy(data.begin(), data.end(), out->begin());
  return true;
}

static void ParseObjectProperty(Value *ret, const json &o) {
  tinygltf::Value::Object vo;
  json::const_iterator it(o.begin());
  json::const_iterator itEnd(o.end());

  for (; it != itEnd; it++) {
    json v = it.value();

    if (v.is_boolean()) {
      vo[it.key()] = tinygltf::Value(v.get<bool>());
    } else if (v.is_number()) {
      vo[it.key()] = tinygltf::Value(v.get<double>());
    } else if (v.is_number_integer()) {
      vo[it.key()] =
          tinygltf::Value(static_cast<int>(v.get<int64_t>()));  // truncate
    } else if (v.is_string()) {
      vo[it.key()] = tinygltf::Value(v.get<std::string>());
    } else if (v.is_object()) {
      tinygltf::Value child_value;
      ParseObjectProperty(&child_value, v);
      vo[it.key()] = child_value;
    }
    // TODO(syoyo) binary, array
  }

  (*ret) = tinygltf::Value(vo);
}

static bool ParseExtrasProperty(Value *ret, const json &o) {
  json::const_iterator it = o.find("extras");
  if (it == o.end()) {
    return false;
  }

  // FIXME(syoyo) Currently we only support `object` type for extras property.
  if (!it.value().is_object()) {
    return false;
  }

  ParseObjectProperty(ret, it.value());

  return true;
}

static bool ParseBooleanProperty(bool *ret, std::string *err,
                                 const json &o,
                                 const std::string &property,
                                 const bool required,
                                 const std::string &parent_node = "") {
  json::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  if (!it.value().is_boolean()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a bool type.\n";
      }
    }
    return false;
  }

  if (ret) {
    (*ret) = it.value().get<bool>();
  }

  return true;
}

static bool ParseNumberProperty(double *ret, std::string *err,
                                const json &o,
                                const std::string &property,
                                const bool required,
                                const std::string &parent_node = "") {
  json::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  if (!it.value().is_number()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a number type.\n";
      }
    }
    return false;
  }

  if (ret) {
    (*ret) = it.value().get<double>();
  }

  return true;
}

static bool ParseNumberArrayProperty(std::vector<double> *ret, std::string *err,
                                     const json &o,
                                     const std::string &property, bool required,
                                     const std::string &parent_node = "") {
  json::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  if (!it.value().is_array()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not an array";
        if (!parent_node.empty()) {
          (*err) += " in " + parent_node;
        }
        (*err) += ".\n";
      }
    }
    return false;
  }

  ret->clear();
  for (json::const_iterator i = it.value().begin(); i != it.value().end(); i++) {
    if (!i.value().is_number()) {
      if (required) {
        if (err) {
          (*err) += "'" + property + "' property is not a number.\n";
          if (!parent_node.empty()) {
            (*err) += " in " + parent_node;
          }
          (*err) += ".\n";
        }
      }
      return false;
    }
    ret->push_back(i.value());
  }

  return true;
}

static bool ParseStringProperty(
    std::string *ret, std::string *err, const json &o,
    const std::string &property, bool required,
    const std::string &parent_node = std::string()) {
  json::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing";
        if (parent_node.empty()) {
          (*err) += ".\n";
        } else {
          (*err) += " in `" + parent_node + "'.\n";
        }
      }
    }
    return false;
  }

  if (!it.value().is_string()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a string type.\n";
      }
    }
    return false;
  }

  if (ret) {
    (*ret) = it.value();
  }

  return true;
}

static bool ParseStringIntProperty(std::map<std::string, int> *ret,
                                   std::string *err, const json &o,
                                   const std::string &property, bool required, const std::string &parent = "") {
  json::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        if (!parent.empty()) {
          (*err) += "'" + property + "' property is missing in " + parent + ".\n";
        } else {
          (*err) += "'" + property + "' property is missing.\n";
        }
      }
    }
    return false;
  }

  // Make sure we are dealing with an object / dictionary.
  if (!it.value().is_object()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not an object.\n";
      }
    }
    return false;
  }

  ret->clear();
  const json &dict = it.value();

  json::const_iterator dictIt(dict.begin());
  json::const_iterator dictItEnd(dict.end());

  for (; dictIt != dictItEnd; ++dictIt) {
    if (!dictIt.value().is_number()) {
      if (required) {
        if (err) {
          (*err) += "'" + property + "' value is not an int.\n";
        }
      }
      return false;
    }

    // Insert into the list.
    (*ret)[dictIt.key()] = static_cast<int>(dictIt.value());
  }
  return true;
}

static bool ParseJSONProperty(std::map<std::string, double> *ret,
                              std::string *err, const json &o,
                              const std::string &property, bool required) {
  json::const_iterator it = o.find(property);
  if (it == o.end()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is missing. \n'";
      }
    }
    return false;
  }

  if (!it.value().is_object()) {
    if (required) {
      if (err) {
        (*err) += "'" + property + "' property is not a JSON object.\n";
      }
    }
    return false;
  }

  ret->clear();
  const json &obj = it.value();
  json::const_iterator it2(obj.begin());
  json::const_iterator itEnd(obj.end());
  for (; it2 != itEnd; it2++) {
    if (it2.value().is_number())
      ret->insert(std::pair<std::string, double>(it2.key(),
                                                 it2.value()));
  }

  return true;
}

static bool ParseAsset(Asset *asset, std::string *err,
                       const json &o) {
  ParseStringProperty(&asset->version, err, o, "version", true, "Asset");
  ParseStringProperty(&asset->generator, err, o, "generator", false, "Asset");
  ParseStringProperty(&asset->minVersion, err, o, "minVersion", false, "Asset");

  // Unity exporter version is added as extra here
  ParseExtrasProperty(&(asset->extras), o);

  return true;
}

static bool ParseImage(Image *image, std::string *err,
                       const json &o, const std::string &basedir,
                       bool is_binary, const unsigned char *bin_data,
                       size_t bin_size) {
  // A glTF image must either reference a bufferView or an image uri
  double bufferView = -1;
  bool isEmbedded =
      ParseNumberProperty(&bufferView, err, o, "bufferView", false);

  std::string uri;
  std::string tmp_err;
  if (!ParseStringProperty(&uri, &tmp_err, o, "uri", false) && !isEmbedded) {
    if (err) {
      (*err) += "`bufferView` or `uri` required for Image.\n";
    }
    return false;
  }

  ParseStringProperty(&image->name, err, o, "name", false);

  std::vector<unsigned char> img;

  if (is_binary) {
    // Still binary glTF accepts external dataURI. First try external resources.
    bool loaded = false;
    if (IsDataURI(uri)) {
      loaded = DecodeDataURI(&img, uri, 0, false);
    } else {
      // Assume external .bin file.
      loaded = LoadExternalFile(&img, err, uri, basedir, 0, false);
    }

    if (!loaded) {
      // load data from (embedded) binary data

      if ((bin_size == 0) || (bin_data == nullptr)) {
        if (err) {
          (*err) += "Invalid binary data.\n";
        }
        return false;
      }

      double buffer_view = -1.0;
      if (!ParseNumberProperty(&buffer_view, err, o, "bufferView", true, "Image")) {
        return false;
      }

      std::string mime_type;
      ParseStringProperty(&mime_type, err, o, "mimeType", false);

      double width = 0.0;
      ParseNumberProperty(&width, err, o, "width", false);

      double height = 0.0;
      ParseNumberProperty(&height, err, o, "height", false);

      // Just only save some information here. Loading actual image data from
      // bufferView is done in other place.
      image->bufferView = static_cast<int>(buffer_view);
      image->mimeType = mime_type;
      image->width = static_cast<int>(width);
      image->height = static_cast<int>(height);

      return true;
    }
  } else {
    if (IsDataURI(uri)) {
      if (!DecodeDataURI(&img, uri, 0, false)) {
        if (err) {
          (*err) += "Failed to decode 'uri' for image parameter.\n";
        }
        return false;
      }
    } else {
      // Assume external file

      // Keep texture path (for textures that cannot be decoded)
      image->uri = uri;

      if (!LoadExternalFile(&img, err, uri, basedir, 0, false)) {
        if (err) {
          (*err) += "Failed to load external 'uri' for image parameter\n";
        }
        // If the image cannot be loaded, keep uri as image->uri.
        return true;
      }
      if (img.empty()) {
        if (err) {
          (*err) += "Image is empty.\n";
        }
        return false;
      }
    }
  }

  return LoadImageData(image, err, 0, 0, &img.at(0),
                       static_cast<int>(img.size()));
}

static bool ParseTexture(Texture *texture, std::string *err,
                         const json &o,
                         const std::string &basedir) {
  (void)basedir;
  double sampler = -1.0;
  double source = -1.0;
  ParseNumberProperty(&sampler, err, o, "sampler", false);

  ParseNumberProperty(&source, err, o, "source", false);

  texture->sampler = static_cast<int>(sampler);
  texture->source = static_cast<int>(source);

  return true;
}

static bool ParseBuffer(Buffer *buffer, std::string *err,
                        const json &o, const std::string &basedir,
                        bool is_binary = false,
                        const unsigned char *bin_data = nullptr,
                        size_t bin_size = 0) {
  double byteLength;
  if (!ParseNumberProperty(&byteLength, err, o, "byteLength", true, "Buffer")) {
    return false;
  }

  // In glTF 2.0, uri is not mandatory anymore
  std::string uri;
  ParseStringProperty(&uri, err, o, "uri", false, "Buffer");

  // having an empty uri for a non embedded image should not be valid
  if (!is_binary && uri.empty()) {
    if (err) {
      (*err) += "'uri' is missing from non binary glTF file buffer.\n";
    }
  }

  json::const_iterator type = o.find("type");
  if (type != o.end()) {
    if (type.value().is_string()) {
      const std::string &ty = type.value();
      if (ty.compare("arraybuffer") == 0) {
        // buffer.type = "arraybuffer";
      }
    }
  }

  size_t bytes = static_cast<size_t>(byteLength);
  if (is_binary) {
    // Still binary glTF accepts external dataURI. First try external resources.

    if (!uri.empty()) {
      // External .bin file.
      LoadExternalFile(&buffer->data, err, uri, basedir, bytes, true);
    } else {
      // load data from (embedded) binary data

      if ((bin_size == 0) || (bin_data == nullptr)) {
        if (err) {
          (*err) += "Invalid binary data in `Buffer'.\n";
        }
        return false;
      }

      if (byteLength > bin_size) {
        if (err) {
          std::stringstream ss;
          ss << "Invalid `byteLength'. Must be equal or less than binary size: "
                "`byteLength' = "
             << byteLength << ", binary size = " << bin_size << std::endl;
          (*err) += ss.str();
        }
        return false;
      }

      // Read buffer data
      buffer->data.resize(static_cast<size_t>(byteLength));
      memcpy(&(buffer->data.at(0)), bin_data, static_cast<size_t>(byteLength));
    }

  } else {
    if (IsDataURI(uri)) {
      if (!DecodeDataURI(&buffer->data, uri, bytes, true)) {
        if (err) {
          (*err) += "Failed to decode 'uri' : " + uri + " in Buffer\n";
        }
        return false;
      }
    } else {
      // Assume external .bin file.
      if (!LoadExternalFile(&buffer->data, err, uri, basedir, bytes, true)) {
        return false;
      }
    }
  }

  ParseStringProperty(&buffer->name, err, o, "name", false);

  return true;
}

static bool ParseBufferView(BufferView *bufferView, std::string *err,
                            const json &o) {
  double buffer = -1.0;
  if (!ParseNumberProperty(&buffer, err, o, "buffer", true, "BufferView")) {
    return false;
  }

  double byteOffset = 0.0;
  ParseNumberProperty(&byteOffset, err, o, "byteOffset", false);

  double byteLength = 1.0;
  if (!ParseNumberProperty(&byteLength, err, o, "byteLength", true,
                           "BufferView")) {
    return false;
  }

  size_t byteStride = 0;
  double byteStrideValue = 0.0;
  if (!ParseNumberProperty(&byteStrideValue, err, o, "byteStride", false)) {
    // Spec says: When byteStride of referenced bufferView is not defined, it
    // means that accessor elements are tightly packed, i.e., effective stride
    // equals the size of the element.
    // We cannot determine the actual byteStride until Accessor are parsed, thus
    // set 0(= tightly packed) here(as done in OpenGL's VertexAttribPoiner)
    byteStride = 0;
  } else {
    byteStride = static_cast<size_t>(byteStrideValue);
  }

  if ((byteStride > 252) || ((byteStride % 4) != 0)) {
    if (err) {
      std::stringstream ss;
      ss << "Invalid `byteStride' value. `byteStride' must be the multiple of "
            "4 : "
         << byteStride << std::endl;

      (*err) += ss.str();
    }
    return false;
  }

  double target = 0.0;
  ParseNumberProperty(&target, err, o, "target", false);
  int targetValue = static_cast<int>(target);
  if ((targetValue == TINYGLTF_TARGET_ARRAY_BUFFER) ||
      (targetValue == TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER)) {
    // OK
  } else {
    targetValue = 0;
  }
  bufferView->target = targetValue;

  ParseStringProperty(&bufferView->name, err, o, "name", false);

  bufferView->buffer = static_cast<int>(buffer);
  bufferView->byteOffset = static_cast<size_t>(byteOffset);
  bufferView->byteLength = static_cast<size_t>(byteLength);
  bufferView->byteStride = static_cast<size_t>(byteStride);

  return true;
}

static bool ParseAccessor(Accessor *accessor, std::string *err,
                          const json &o) {
  double bufferView = -1.0;
  if (!ParseNumberProperty(&bufferView, err, o, "bufferView", true,
                           "Accessor")) {
    return false;
  }

  double byteOffset = 0.0;
  ParseNumberProperty(&byteOffset, err, o, "byteOffset", false, "Accessor");

  bool normalized = false;
  ParseBooleanProperty(&normalized, err, o, "normalized", false, "Accessor");

  double componentType = 0.0;
  if (!ParseNumberProperty(&componentType, err, o, "componentType", true,
                           "Accessor")) {
    return false;
  }

  double count = 0.0;
  if (!ParseNumberProperty(&count, err, o, "count", true, "Accessor")) {
    return false;
  }

  std::string type;
  if (!ParseStringProperty(&type, err, o, "type", true, "Accessor")) {
    return false;
  }

  if (type.compare("SCALAR") == 0) {
    accessor->type = TINYGLTF_TYPE_SCALAR;
  } else if (type.compare("VEC2") == 0) {
    accessor->type = TINYGLTF_TYPE_VEC2;
  } else if (type.compare("VEC3") == 0) {
    accessor->type = TINYGLTF_TYPE_VEC3;
  } else if (type.compare("VEC4") == 0) {
    accessor->type = TINYGLTF_TYPE_VEC4;
  } else if (type.compare("MAT2") == 0) {
    accessor->type = TINYGLTF_TYPE_MAT2;
  } else if (type.compare("MAT3") == 0) {
    accessor->type = TINYGLTF_TYPE_MAT3;
  } else if (type.compare("MAT4") == 0) {
    accessor->type = TINYGLTF_TYPE_MAT4;
  } else {
    std::stringstream ss;
    ss << "Unsupported `type` for accessor object. Got \"" << type << "\"\n";
    if (err) {
      (*err) += ss.str();
    }
    return false;
  }

  ParseStringProperty(&accessor->name, err, o, "name", false);

  accessor->minValues.clear();
  accessor->maxValues.clear();
  ParseNumberArrayProperty(&accessor->minValues, err, o, "min", false,
                           "Accessor");

  ParseNumberArrayProperty(&accessor->maxValues, err, o, "max", false,
                           "Accessor");

  accessor->count = static_cast<size_t>(count);
  accessor->bufferView = static_cast<int>(bufferView);
  accessor->byteOffset = static_cast<size_t>(byteOffset);
  accessor->normalized = normalized;
  {
    int comp = static_cast<int>(componentType);
    if (comp >= TINYGLTF_COMPONENT_TYPE_BYTE &&
        comp <= TINYGLTF_COMPONENT_TYPE_DOUBLE) {
      // OK
      accessor->componentType = comp;
    } else {
      std::stringstream ss;
      ss << "Invalid `componentType` in accessor. Got " << comp << "\n";
      if (err) {
        (*err) += ss.str();
      }
      return false;
    }
  }

  ParseExtrasProperty(&(accessor->extras), o);

  return true;
}

static bool ParsePrimitive(Primitive *primitive, std::string *err,
                           const json &o) {
  double material = -1.0;
  ParseNumberProperty(&material, err, o, "material", false);
  primitive->material = static_cast<int>(material);

  double mode = static_cast<double>(TINYGLTF_MODE_TRIANGLES);
  ParseNumberProperty(&mode, err, o, "mode", false);

  int primMode = static_cast<int>(mode);
  primitive->mode = primMode;  // Why only triangled were supported ?

  double indices = -1.0;
  ParseNumberProperty(&indices, err, o, "indices", false);
  primitive->indices = static_cast<int>(indices);
  if (!ParseStringIntProperty(&primitive->attributes, err, o, "attributes",
                              true, "Primitive")) {
    return false;
  }

  // Look for morph targets
  json::const_iterator targetsObject = o.find("targets");
  if ((targetsObject != o.end()) &&
      targetsObject.value().is_array()) {
    for (json::const_iterator i = targetsObject.value().begin(); i != targetsObject.value().end(); i++) {
      std::map<std::string, int> targetAttribues;

      const json &dict = i.value();
      json::const_iterator dictIt(dict.begin());
      json::const_iterator dictItEnd(dict.end());

      for (; dictIt != dictItEnd; ++dictIt) {
        targetAttribues[dictIt.key()] =
            static_cast<int>(dictIt.value());
      }
      primitive->targets.push_back(targetAttribues);
    }
  }

  ParseExtrasProperty(&(primitive->extras), o);

  return true;
}

static bool ParseMesh(Mesh *mesh, std::string *err, const json &o) {
  ParseStringProperty(&mesh->name, err, o, "name", false);

  mesh->primitives.clear();
  json::const_iterator primObject = o.find("primitives");
  if ((primObject != o.end()) && primObject.value().is_array()) {
    for (json::const_iterator i = primObject.value().begin(); i != primObject.value().end(); i++) {
      Primitive primitive;
      if (ParsePrimitive(&primitive, err,
                         i.value())) {
        // Only add the primitive if the parsing succeeds.
        mesh->primitives.push_back(primitive);
      }
    }
  }

  // Look for morph targets
  json::const_iterator targetsObject = o.find("targets");
  if ((targetsObject != o.end()) &&
      targetsObject.value().is_array()) {
    for (json::const_iterator i = targetsObject.value().begin(); i != targetsObject.value().end(); i++) {
      std::map<std::string, int> targetAttribues;

      const json &dict = i.value();
      json::const_iterator dictIt(dict.begin());
      json::const_iterator dictItEnd(dict.end());

      for (; dictIt != dictItEnd; ++dictIt) {
        targetAttribues[dictIt.key()] =
            static_cast<int>(dictIt.value());
      }
      mesh->targets.push_back(targetAttribues);
    }
  }

  // Should probably check if has targets and if dimensions fit
  ParseNumberArrayProperty(&mesh->weights, err, o, "weights", false);

  ParseExtrasProperty(&(mesh->extras), o);

  return true;
}

static bool ParseParameterProperty(Parameter *param, std::string *err,
                                   const json &o,
                                   const std::string &prop, bool required) {
  double num_val;

  // A parameter value can either be a string or an array of either a boolean or
  // a number. Booleans of any kind aren't supported here. Granted, it
  // complicates the Parameter structure and breaks it semantically in the sense
  // that the client probably works off the assumption that if the string is
  // empty the vector is used, etc. Would a tagged union work?
  if (ParseStringProperty(&param->string_value, err, o, prop, false)) {
    // Found string property.
    return true;
  } else if (ParseNumberArrayProperty(&param->number_array, err, o, prop,
                                      false)) {
    // Found a number array.
    return true;
  } else if (ParseNumberProperty(&num_val, err, o, prop, false)) {
    param->number_array.push_back(num_val);
    return true;
  } else if (ParseJSONProperty(&param->json_double_value, err, o, prop,
                               false)) {
    return true;
  } else if (ParseBooleanProperty(&param->bool_value, err, o, prop, false)) {
    return true;
  } else {
    if (required) {
      if (err) {
        (*err) += "parameter must be a string or number / number array.\n";
      }
    }
    return false;
  }
}

static bool ParseLight(Light *light, std::string *err, const json &o) {
  ParseStringProperty(&light->name, err, o, "name", false);
  ParseNumberArrayProperty(&light->color, err, o, "color", false);
  ParseStringProperty(&light->type, err, o, "type", false);
  return true;
}

static bool ParseNode(Node *node, std::string *err, const json &o) {
  ParseStringProperty(&node->name, err, o, "name", false);

  double skin = -1.0;
  ParseNumberProperty(&skin, err, o, "skin", false);
  node->skin = static_cast<int>(skin);

  // Matrix and T/R/S are exclusive
  if (!ParseNumberArrayProperty(&node->matrix, err, o, "matrix", false)) {
    ParseNumberArrayProperty(&node->rotation, err, o, "rotation", false);
    ParseNumberArrayProperty(&node->scale, err, o, "scale", false);
    ParseNumberArrayProperty(&node->translation, err, o, "translation", false);
  }

  double camera = -1.0;
  ParseNumberProperty(&camera, err, o, "camera", false);
  node->camera = static_cast<int>(camera);

  double mesh = -1.0;
  ParseNumberProperty(&mesh, err, o, "mesh", false);
  node->mesh = int(mesh);

  node->children.clear();
  json::const_iterator childrenObject = o.find("children");
  if ((childrenObject != o.end()) &&
      childrenObject.value().is_array()) {
    for (json::const_iterator i = childrenObject.value().begin(); i != childrenObject.value().end(); i++) {
      if (!i.value().is_number()) {
        if (err) {
          (*err) += "Invalid `children` array.\n";
        }
        return false;
      }
      const int &childrenNode =
          static_cast<int>(i.value());
      node->children.push_back(childrenNode);
    }
  }

  ParseExtrasProperty(&(node->extras), o);

  json::const_iterator extensions_object = o.find("extensions");
  if ((extensions_object != o.end()) &&
      extensions_object.value().is_object()) {
    const json &ext_values_object =
      extensions_object.value();

    json::const_iterator it(ext_values_object.begin());
    json::const_iterator itEnd(ext_values_object.end());

    for (; it != itEnd; it++) {
      if ((it.key().compare("KHR_lights_cmn") == 0) &&
         it.value().is_object()) {
        const json &light_values_object =
          it.value();

        json::const_iterator itVal(light_values_object.begin());
        json::const_iterator itValEnd(light_values_object.end());

        for (; itVal != itValEnd; itVal++) {
          Parameter param;
          if (ParseParameterProperty(&param, err, light_values_object, itVal.key(),
                false)) {
            node->extLightsValues[itVal.key()] = param;
          }
        }
      }
    }
  }

  return true;
}

static bool ParseMaterial(Material *material, std::string *err,
                          const json &o) {
  material->values.clear();
  material->extPBRValues.clear();
  material->additionalValues.clear();

  json::const_iterator it(o.begin());
  json::const_iterator itEnd(o.end());

  for (; it != itEnd; it++) {
    if (it.key() == "pbrMetallicRoughness") {
      if (it.value().is_object()) {
        const json &values_object =
            it.value();

        json::const_iterator itVal(values_object.begin());
        json::const_iterator itValEnd(values_object.end());

        for (; itVal != itValEnd; itVal++) {
          Parameter param;
          if (ParseParameterProperty(&param, err, values_object, itVal.key(),
                                     false)) {
            material->values[itVal.key()] = param;
          }
        }
      }
    } else if (it.key() == "extensions") {
      if (it.value().is_object()) {
        const json &extension =
            it.value();

        json::const_iterator extIt = extension.begin();
        if (!extIt.value().is_object()) continue;

        const json &values_object =
            extIt.value();

        json::const_iterator itVal(values_object.begin());
        json::const_iterator itValEnd(values_object.end());

        for (; itVal != itValEnd; itVal++) {
          Parameter param;
          if (ParseParameterProperty(&param, err, values_object, itVal.key(),
                                     false)) {
            material->extPBRValues[itVal.key()] = param;
          }
        }
      }
    } else {
      Parameter param;
      if (ParseParameterProperty(&param, err, o, it.key(), false)) {
        material->additionalValues[it.key()] = param;
      }
    }
  }

  ParseExtrasProperty(&(material->extras), o);

  return true;
}

static bool ParseAnimationChannel(AnimationChannel *channel, std::string *err,
                                  const json &o) {
  double samplerIndex = -1.0;
  double targetIndex = -1.0;
  if (!ParseNumberProperty(&samplerIndex, err, o, "sampler", true, "AnimationChannel")) {
    if (err) {
      (*err) += "`sampler` field is missing in animation channels\n";
    }
    return false;
  }

  json::const_iterator targetIt = o.find("target");
  if ((targetIt != o.end()) && targetIt.value().is_object()) {
    const json &target_object =
        targetIt.value();

    if (!ParseNumberProperty(&targetIndex, err, target_object, "node", true)) {
      if (err) {
        (*err) += "`node` field is missing in animation.channels.target\n";
      }
      return false;
    }

    if (!ParseStringProperty(&channel->target_path, err, target_object, "path",
                             true)) {
      if (err) {
        (*err) += "`path` field is missing in animation.channels.target\n";
      }
      return false;
    }
  }

  channel->sampler = static_cast<int>(samplerIndex);
  channel->target_node = static_cast<int>(targetIndex);

  ParseExtrasProperty(&(channel->extras), o);

  return true;
}

static bool ParseAnimation(Animation *animation, std::string *err,
                           const json &o) {
  {
    json::const_iterator channelsIt = o.find("channels");
    if ((channelsIt != o.end()) && channelsIt.value().is_array()) {
      for (json::const_iterator i = channelsIt.value().begin(); i != channelsIt.value().end(); i++) {
        AnimationChannel channel;
        if (ParseAnimationChannel(&channel, err,
                                  i.value())) {
          // Only add the channel if the parsing succeeds.
          animation->channels.push_back(channel);
        }
      }
    }
  }

  {
    json::const_iterator samplerIt = o.find("samplers");
    if ((samplerIt != o.end()) && samplerIt.value().is_array()) {
      const json &sampler_array =
          samplerIt.value();

      json::const_iterator it = sampler_array.begin();
      json::const_iterator itEnd = sampler_array.end();

      for (; it != itEnd; it++) {
        const json &s = it->get<json>();

        AnimationSampler sampler;
        double inputIndex = -1.0;
        double outputIndex = -1.0;
        if (!ParseNumberProperty(&inputIndex, err, s, "input", true)) {
          if (err) {
            (*err) += "`input` field is missing in animation.sampler\n";
          }
          return false;
        }
        if (!ParseStringProperty(&sampler.interpolation, err, s,
                                 "interpolation", true)) {
          if (err) {
            (*err) += "`interpolation` field is missing in animation.sampler\n";
          }
          return false;
        }
        if (!ParseNumberProperty(&outputIndex, err, s, "output", true)) {
          if (err) {
            (*err) += "`output` field is missing in animation.sampler\n";
          }
          return false;
        }
        sampler.input = static_cast<int>(inputIndex);
        sampler.output = static_cast<int>(outputIndex);
        animation->samplers.push_back(sampler);
      }
    }
  }

  ParseStringProperty(&animation->name, err, o, "name", false);

  ParseExtrasProperty(&(animation->extras), o);

  return true;
}

static bool ParseSampler(Sampler *sampler, std::string *err,
                         const json &o) {
  ParseStringProperty(&sampler->name, err, o, "name", false);

  double minFilter =
      static_cast<double>(TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR);
  double magFilter = static_cast<double>(TINYGLTF_TEXTURE_FILTER_LINEAR);
  double wrapS = static_cast<double>(TINYGLTF_TEXTURE_WRAP_REPEAT);
  double wrapT = static_cast<double>(TINYGLTF_TEXTURE_WRAP_REPEAT);
  ParseNumberProperty(&minFilter, err, o, "minFilter", false);
  ParseNumberProperty(&magFilter, err, o, "magFilter", false);
  ParseNumberProperty(&wrapS, err, o, "wrapS", false);
  ParseNumberProperty(&wrapT, err, o, "wrapT", false);

  sampler->minFilter = static_cast<int>(minFilter);
  sampler->magFilter = static_cast<int>(magFilter);
  sampler->wrapS = static_cast<int>(wrapS);
  sampler->wrapT = static_cast<int>(wrapT);

  ParseExtrasProperty(&(sampler->extras), o);

  return true;
}

static bool ParseSkin(Skin *skin, std::string *err, const json &o) {
  ParseStringProperty(&skin->name, err, o, "name", false, "Skin");

  std::vector<double> joints;
  if (!ParseNumberArrayProperty(&joints, err, o, "joints", false, "Skin")) {
    return false;
  }

  double skeleton = -1.0;
  ParseNumberProperty(&skeleton, err, o, "skeleton", false, "Skin");
  skin->skeleton = static_cast<int>(skeleton);

  skin->joints.resize(joints.size());
  for (size_t i = 0; i < joints.size(); i++) {
    skin->joints[i] = static_cast<int>(joints[i]);
  }

  double invBind = -1.0;
  ParseNumberProperty(&invBind, err, o, "inverseBindMatrices", true, "Skin");
  skin->inverseBindMatrices = static_cast<int>(invBind);

  return true;
}

static bool ParsePerspectiveCamera(PerspectiveCamera *camera, std::string *err,
                                   const json &o) {
  double yfov = 0.0;
  if (!ParseNumberProperty(&yfov, err, o, "yfov", true, "OrthographicCamera")) {
    return false;
  }

  double znear = 0.0;
  if (!ParseNumberProperty(&znear, err, o, "znear", true,
                           "PerspectiveCamera")) {
    return false;
  }

  double aspectRatio = 0.0;  // = invalid
  ParseNumberProperty(&aspectRatio, err, o, "aspectRatio", false,
                      "PerspectiveCamera");

  double zfar = 0.0;  // = invalid
  ParseNumberProperty(&zfar, err, o, "zfar", false, "PerspectiveCamera");

  camera->aspectRatio = float(aspectRatio);
  camera->zfar = float(zfar);
  camera->yfov = float(yfov);
  camera->znear = float(znear);

  ParseExtrasProperty(&(camera->extras), o);

  // TODO(syoyo): Validate parameter values.

  return true;
}

static bool ParseOrthographicCamera(OrthographicCamera *camera,
                                    std::string *err,
                                    const json &o) {
  double xmag = 0.0;
  if (!ParseNumberProperty(&xmag, err, o, "xmag", true, "OrthographicCamera")) {
    return false;
  }

  double ymag = 0.0;
  if (!ParseNumberProperty(&ymag, err, o, "ymag", true, "OrthographicCamera")) {
    return false;
  }

  double zfar = 0.0;
  if (!ParseNumberProperty(&zfar, err, o, "zfar", true, "OrthographicCamera")) {
    return false;
  }

  double znear = 0.0;
  if (!ParseNumberProperty(&znear, err, o, "znear", true,
                           "OrthographicCamera")) {
    return false;
  }

  ParseExtrasProperty(&(camera->extras), o);

  camera->xmag = float(xmag);
  camera->ymag = float(ymag);
  camera->zfar = float(zfar);
  camera->znear = float(znear);

  // TODO(syoyo): Validate parameter values.

  return true;
}

static bool ParseCamera(Camera *camera, std::string *err,
                        const json &o) {
  if (!ParseStringProperty(&camera->type, err, o, "type", true, "Camera")) {
    return false;
  }

  if (camera->type.compare("orthographic") == 0) {
    if (o.find("orthographic") == o.end()) {
      if (err) {
        std::stringstream ss;
        ss << "Orhographic camera description not found." << std::endl;
        (*err) += ss.str();
      }
      return false;
    }

    const json &v = o.find("orthographic").value();
    if (!v.is_object()) {
      if (err) {
        std::stringstream ss;
        ss << "\"orthographic\" is not a JSON object." << std::endl;
        (*err) += ss.str();
      }
      return false;
    }

    if (!ParseOrthographicCamera(&camera->orthographic, err,
                                 v.get<json>())) {
      return false;
    }
  } else if (camera->type.compare("perspective") == 0) {
    if (o.find("perspective") == o.end()) {
      if (err) {
        std::stringstream ss;
        ss << "Perspective camera description not found." << std::endl;
        (*err) += ss.str();
      }
      return false;
    }

    const json &v = o.find("perspective").value();
    if (!v.is_object()) {
      if (err) {
        std::stringstream ss;
        ss << "\"perspective\" is not a JSON object." << std::endl;
        (*err) += ss.str();
      }
      return false;
    }

    if (!ParsePerspectiveCamera(&camera->perspective, err,
                                v.get<json>())) {
      return false;
    }
  } else {
    if (err) {
      std::stringstream ss;
      ss << "Invalid camera type: \"" << camera->type
         << "\". Must be \"perspective\" or \"orthographic\"" << std::endl;
      (*err) += ss.str();
    }
    return false;
  }

  ParseStringProperty(&camera->name, err, o, "name", false);

  ParseExtrasProperty(&(camera->extras), o);

  return true;
}

bool TinyGLTF::LoadFromString(Model *model, std::string *err, const char *str,
                              unsigned int length, const std::string &base_dir,
                              unsigned int check_sections) {
  if (length < 4) {
    if (err) {
      (*err) = "JSON string too short.\n";
    }
    return false;
  }

  json v;

#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && not defined(TINYGLTF_NOEXCEPTION)
  try {
    v = json::parse(str, str + length);

  } catch (const std::exception &e) {
    if (err) {
      (*err) = e.what();
    }
    return false;
  }
#else
  {
    v = json::parse(str, str + length, nullptr, /* exception */false);

    if (!v.is_object()) {
      // Assume parsing was failed.
      if (err) {
        (*err) = "Failed to parse JSON object\n";
      }
      return false;
    }
  }
#endif

  if (!v.is_object()) {
    // root is not an object.
    if (err) {
      (*err) = "Root element is not a JSON object\n";
    }
    return false;
  }

  // scene is not mandatory.
  // FIXME Maybe a better way to handle it than removing the code

  {
    json::const_iterator it = v.find("scenes"); 
    if ((it != v.end()) && it.value().is_array()) {
      // OK
    } else if (check_sections & REQUIRE_SCENES) {
      if (err) {
        (*err) += "\"scenes\" object not found in .gltf or not an array type\n";
      }
      return false;
    }
  }

  {
    json::const_iterator it = v.find("nodes"); 
    if ((it != v.end()) && it.value().is_array()) {
      // OK
    } else if (check_sections & REQUIRE_NODES) {
      if (err) {
        (*err) += "\"nodes\" object not found in .gltf\n";
      }
      return false;
    }
  }

  {
    json::const_iterator it = v.find("accessors"); 
    if ((it != v.end()) && it.value().is_array()) {
      // OK
    } else if (check_sections & REQUIRE_ACCESSORS) {
      if (err) {
        (*err) += "\"accessors\" object not found in .gltf\n";
      }
      return false;
    }
  }

  {
    json::const_iterator it = v.find("buffers"); 
    if ((it != v.end()) && it.value().is_array()) {
      // OK
    } else if (check_sections & REQUIRE_BUFFERS) {
      if (err) {
        (*err) += "\"buffers\" object not found in .gltf\n";
      }
      return false;
    }
  }

  {
    json::const_iterator it = v.find("bufferViews"); 
    if ((it != v.end()) && it.value().is_array()) {
      // OK
    } else if (check_sections & REQUIRE_BUFFER_VIEWS) {
      if (err) {
        (*err) += "\"bufferViews\" object not found in .gltf\n";
      }
      return false;
    }
  }

  model->buffers.clear();
  model->bufferViews.clear();
  model->accessors.clear();
  model->meshes.clear();
  model->cameras.clear();
  model->nodes.clear();
  model->extensionsUsed.clear();
  model->extensionsRequired.clear();
  model->defaultScene = -1;

  // 1. Parse Asset
  {
    json::const_iterator it = v.find("asset"); 
    if ((it != v.end()) && it.value().is_object()) {
      const json &root = it.value();

      ParseAsset(&model->asset, err, root);
    }
  }

  // 2. Parse extensionUsed
  {
    json::const_iterator it = v.find("extensionsUsed"); 
    if ((it != v.end()) && it.value().is_array()) {
      const json &root = it.value();
      for (unsigned int i = 0; i < root.size(); ++i) {
        model->extensionsUsed.push_back(root[i].get<std::string>());
      }
    }
  }

  {
    json::const_iterator it = v.find("extensionsRequired"); 
    if ((it != v.end()) && it.value().is_array()) {
      const json &root = it.value();
      for (unsigned int i = 0; i < root.size(); ++i) {
        model->extensionsRequired.push_back(root[i].get<std::string>());
      }
    }
  }

  // 3. Parse Buffer
  {
    json::const_iterator rootIt = v.find("buffers"); 
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`buffers' does not contain an JSON object.";
          }
          return false;
        }
        Buffer buffer;
        if (!ParseBuffer(&buffer, err, it->get<json>(), base_dir,
                         is_binary_, bin_data_, bin_size_)) {
          return false;
        }

        model->buffers.push_back(buffer);
      }
    }
  }
  
  // 4. Parse BufferView
  {
    json::const_iterator rootIt = v.find("bufferViews"); 
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`bufferViews' does not contain an JSON object.";
          }
          return false;
        }
        BufferView bufferView;
        if (!ParseBufferView(&bufferView, err, it->get<json>())) {
          return false;
        }

        model->bufferViews.push_back(bufferView);
      }
    }
  }

  // 5. Parse Accessor
  {
    json::const_iterator rootIt = v.find("accessors"); 
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`accessors' does not contain an JSON object.";
          }
          return false;
        }
        Accessor accessor;
        if (!ParseAccessor(&accessor, err, it->get<json>())) {
          return false;
        }

        model->accessors.push_back(accessor);
      }
    }
  }

  // 6. Parse Mesh
  {
    json::const_iterator rootIt = v.find("meshes"); 
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`meshes' does not contain an JSON object.";
          }
          return false;
        }
        Mesh mesh;
        if (!ParseMesh(&mesh, err, it->get<json>())) {
          return false;
        }

        model->meshes.push_back(mesh);
      }
    }
  }

  // 7. Parse Node
  {
    json::const_iterator rootIt = v.find("nodes"); 
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`nodes' does not contain an JSON object.";
          }
          return false;
        }
        Node node;
        if (!ParseNode(&node, err, it->get<json>())) {
          return false;
        }

        model->nodes.push_back(node);
      }
    }
  }

  // 8. Parse scenes.
  {
    json::const_iterator rootIt = v.find("scenes"); 
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();


      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!(it.value().is_object())) {
          if (err) {
            (*err) += "`scenes' does not contain an JSON object.";
          }
          return false;
        }
        const json &o = it->get<json>();
        std::vector<double> nodes;
        if (!ParseNumberArrayProperty(&nodes, err, o, "nodes", false)) {
          return false;
        }

        Scene scene;
        ParseStringProperty(&scene.name, err, o, "name", false);
        std::vector<int> nodesIds;
        for (size_t i = 0; i < nodes.size(); i++) {
          nodesIds.push_back(static_cast<int>(nodes[i]));
        }
        scene.nodes = nodesIds;

        model->scenes.push_back(scene);
      }
    }
  }

  // 9. Parse default scenes.
  {
    json::const_iterator rootIt = v.find("scene"); 
    if ((rootIt != v.end()) && rootIt.value().is_number_integer()) {
      const int defaultScene = rootIt.value();

      model->defaultScene = static_cast<int>(defaultScene);
    }
  }

  // 10. Parse Material
  {
    json::const_iterator rootIt = v.find("materials"); 
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`materials' does not contain an JSON object.";
          }
          return false;
        }
        json jsonMaterial = it->get<json>();

        Material material;
        ParseStringProperty(&material.name, err, jsonMaterial, "name", false);

        if (!ParseMaterial(&material, err, jsonMaterial)) {
          return false;
        }

        model->materials.push_back(material);
      }
    }
  }

  // 11. Parse Image
  {
    json::const_iterator rootIt = v.find("images"); 
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`images' does not contain an JSON object.";
          }
          return false;
        }
        Image image;
        if (!ParseImage(&image, err, it.value(), base_dir,
                        is_binary_, bin_data_, bin_size_)) {
          return false;
        }

        if (image.bufferView != -1) {
          // Load image from the buffer view.
          if (size_t(image.bufferView) >= model->bufferViews.size()) {
            if (err) {
              std::stringstream ss;
              ss << "bufferView \"" << image.bufferView
                 << "\" not found in the scene." << std::endl;
              (*err) += ss.str();
            }
            return false;
          }

          const BufferView &bufferView =
              model->bufferViews[size_t(image.bufferView)];
          const Buffer &buffer = model->buffers[size_t(bufferView.buffer)];

          bool ret = LoadImageData(&image, err, image.width, image.height,
                                   &buffer.data[bufferView.byteOffset],
                                   static_cast<int>(bufferView.byteLength));
          if (!ret) {
            return false;
          }
        }

        model->images.push_back(image);
      }
    }
  }

  // 12. Parse Texture
  {
    json::const_iterator rootIt = v.find("textures"); 
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; it++) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`textures' does not contain an JSON object.";
          }
          return false;
        }
        Texture texture;
        if (!ParseTexture(&texture, err, it->get<json>(), base_dir)) {
          return false;
        }

        model->textures.push_back(texture);
      }
    }
  }

  // 13. Parse Animation
  {
    json::const_iterator rootIt = v.find("animations"); 
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; ++it) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`animations' does not contain an JSON object.";
          }
          return false;
        }
        Animation animation;
        if (!ParseAnimation(&animation, err, it->get<json>())) {
          return false;
        }

        model->animations.push_back(animation);
      }
    }
  }

  // 14. Parse Skin
  {
    json::const_iterator rootIt = v.find("skins"); 
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; ++it) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`skins' does not contain an JSON object.";
          }
          return false;
        }
        Skin skin;
        if (!ParseSkin(&skin, err, it->get<json>())) {
          return false;
        }

        model->skins.push_back(skin);
      }
    }
  }

  // 15. Parse Sampler
  {
    json::const_iterator rootIt = v.find("samplers"); 
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; ++it) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`samplers' does not contain an JSON object.";
          }
          return false;
        }
        Sampler sampler;
        if (!ParseSampler(&sampler, err, it->get<json>())) {
          return false;
        }

        model->samplers.push_back(sampler);
      }
    }
  }

  // 16. Parse Camera
  {
    json::const_iterator rootIt = v.find("cameras"); 
    if ((rootIt != v.end()) && rootIt.value().is_array()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; ++it) {
        if (!it.value().is_object()) {
          if (err) {
            (*err) += "`cameras' does not contain an JSON object.";
          }
          return false;
        }
        Camera camera;
        if (!ParseCamera(&camera, err, it->get<json>())) {
          return false;
        }

        model->cameras.push_back(camera);
      }
    }
  }

  // 17. Parse Extensions
  {
    json::const_iterator rootIt = v.find("extensions"); 
    if ((rootIt != v.end()) && rootIt.value().is_object()) {
      const json &root = rootIt.value();

      json::const_iterator it(root.begin());
      json::const_iterator itEnd(root.end());
      for (; it != itEnd; ++it) {
        // parse KHR_lights_cmn extension
        if ((it.key().compare("KHR_lights_cmn") == 0) && it.value().is_object()) {
          const json &object = it.value();
          json::const_iterator itLight(object.find("lights"));
          json::const_iterator itLightEnd(object.end());
          if (itLight == itLightEnd) {
            continue;
          }

          if (!itLight.value().is_array()) {
            continue;
          }

          const json &lights = itLight.value();
          json::const_iterator arrayIt(lights.begin());
          json::const_iterator arrayItEnd(lights.end());
          for (; arrayIt != arrayItEnd; ++arrayIt) {
            Light light;
            if (!ParseLight(&light, err, arrayIt.value())) {
              return false;
            }
            model->lights.push_back(light);
          }
        }
      }
    }
  }

  return true;
}

bool TinyGLTF::LoadASCIIFromString(Model *model, std::string *err,
                                   const char *str, unsigned int length,
                                   const std::string &base_dir,
                                   unsigned int check_sections) {
  is_binary_ = false;
  bin_data_ = nullptr;
  bin_size_ = 0;

  return LoadFromString(model, err, str, length, base_dir, check_sections);
}

bool TinyGLTF::LoadASCIIFromFile(Model *model, std::string *err,
                                 const std::string &filename,
                                 unsigned int check_sections) {
  std::stringstream ss;

  std::ifstream f(filename.c_str());
  if (!f) {
    ss << "Failed to open file: " << filename << std::endl;
    if (err) {
      (*err) = ss.str();
    }
    return false;
  }

  f.seekg(0, f.end);
  size_t sz = static_cast<size_t>(f.tellg());
  std::vector<char> buf(sz);

  if (sz == 0) {
    if (err) {
      (*err) = "Empty file.";
    }
    return false;
  }

  f.seekg(0, f.beg);
  f.read(&buf.at(0), static_cast<std::streamsize>(sz));
  f.close();

  std::string basedir = GetBaseDir(filename);

  bool ret = LoadASCIIFromString(model, err, &buf.at(0),
                                 static_cast<unsigned int>(buf.size()), basedir,
                                 check_sections);

  return ret;
}

bool TinyGLTF::LoadBinaryFromMemory(Model *model, std::string *err,
                                    const unsigned char *bytes,
                                    unsigned int size,
                                    const std::string &base_dir,
                                    unsigned int check_sections) {
  if (size < 20) {
    if (err) {
      (*err) = "Too short data size for glTF Binary.";
    }
    return false;
  }

  if (bytes[0] == 'g' && bytes[1] == 'l' && bytes[2] == 'T' &&
      bytes[3] == 'F') {
    // ok
  } else {
    if (err) {
      (*err) = "Invalid magic.";
    }
    return false;
  }

  unsigned int version;       // 4 bytes
  unsigned int length;        // 4 bytes
  unsigned int model_length;  // 4 bytes
  unsigned int model_format;  // 4 bytes;

  // @todo { Endian swap for big endian machine. }
  memcpy(&version, bytes + 4, 4);
  swap4(&version);
  memcpy(&length, bytes + 8, 4);
  swap4(&length);
  memcpy(&model_length, bytes + 12, 4);
  swap4(&model_length);
  memcpy(&model_format, bytes + 16, 4);
  swap4(&model_format);

  if ((20 + model_length >= size) || (model_length < 1) ||
      (model_format != 0x4E4F534A)) {  // 0x4E4F534A = JSON format.
    if (err) {
      (*err) = "Invalid glTF binary.";
    }
    return false;
  }

  // Extract JSON string.
  std::string jsonString(reinterpret_cast<const char *>(&bytes[20]),
                         model_length);

  is_binary_ = true;
  bin_data_ = bytes + 20 + model_length +
              8;  // 4 bytes (buffer_length) + 4 bytes(buffer_format)
  bin_size_ =
      length - (20 + model_length);  // extract header + JSON scene data.

  bool ret =
      LoadFromString(model, err, reinterpret_cast<const char *>(&bytes[20]),
                     model_length, base_dir, check_sections);
  if (!ret) {
    return ret;
  }

  return true;
}

bool TinyGLTF::LoadBinaryFromFile(Model *model, std::string *err,
                                  const std::string &filename,
                                  unsigned int check_sections) {
  std::stringstream ss;

  std::ifstream f(filename.c_str(), std::ios::binary);
  if (!f) {
    ss << "Failed to open file: " << filename << std::endl;
    if (err) {
      (*err) = ss.str();
    }
    return false;
  }

  f.seekg(0, f.end);
  size_t sz = static_cast<size_t>(f.tellg());
  std::vector<char> buf(sz);

  f.seekg(0, f.beg);
  f.read(&buf.at(0), static_cast<std::streamsize>(sz));
  f.close();

  std::string basedir = GetBaseDir(filename);

  bool ret = LoadBinaryFromMemory(
      model, err, reinterpret_cast<unsigned char *>(&buf.at(0)),
      static_cast<unsigned int>(buf.size()), basedir, check_sections);

  return ret;
}

///////////////////////
// GLTF Serialization
///////////////////////

//typedef std::pair<std::string, json> json_object_pair;

template <typename T>
static void SerializeNumberProperty(const std::string &key, T number,
                                    json &obj) {
  //obj.insert(
  //    json_object_pair(key, json(static_cast<double>(number))));
  obj[key] = static_cast<double>(number);
}

template <typename T>
static void SerializeNumberArrayProperty(const std::string &key,
                                         const std::vector<T> &value,
                                         json &obj) {
  json o;
  json vals;

  for (unsigned int i = 0; i < value.size(); ++i) {
    vals.push_back(static_cast<double>(value[i]));
  }

  obj[key] = vals;
}

static void SerializeStringProperty(const std::string &key,
                                    const std::string &value,
                                    json &obj) {
  obj[key] = value;
}

static void SerializeStringArrayProperty(const std::string &key,
                                         const std::vector<std::string> &value,
                                         json &obj) {
  json o;
  json vals;

  for (unsigned int i = 0; i < value.size(); ++i) {
    vals.push_back(value[i]);
  }

  obj[key] = vals;
}

static void SerializeValue(const std::string &key, const Value &value,
                           json &obj) {
  if (value.IsArray()) {
    json jsonValue;
    for (unsigned int i = 0; i < value.ArrayLen(); ++i) {
      Value elementValue = value.Get(int(i));
      if (elementValue.IsString())
        jsonValue.push_back(elementValue.Get<std::string>());
    }
    obj[key] = jsonValue;
  } else {
    json jsonValue;
    std::vector<std::string> valueKeys;
    for (unsigned int i = 0; i < valueKeys.size(); ++i) {
      Value elementValue = value.Get(valueKeys[i]);
      if (elementValue.IsInt())
        jsonValue[valueKeys[i]] = 
            static_cast<double>(elementValue.Get<int>());
    }

    obj[key] = jsonValue;
  }
}

static void SerializeGltfBufferData(const std::vector<unsigned char> &data,
                                    const std::string &binFilePath) {
  std::ofstream output(binFilePath.c_str(), std::ofstream::binary);
  output.write(reinterpret_cast<const char *>(&data[0]),
               std::streamsize(data.size()));
  output.close();
}

static void SerializeParameterMap(ParameterMap &param, json &o) {
  for (ParameterMap::iterator paramIt = param.begin(); paramIt != param.end();
       ++paramIt) {
    if (paramIt->second.number_array.size()) {
      SerializeNumberArrayProperty<double>(paramIt->first,
                                           paramIt->second.number_array, o);
    } else if (paramIt->second.json_double_value.size()) {
      json json_double_value;

      for (std::map<std::string, double>::iterator it =
               paramIt->second.json_double_value.begin();
           it != paramIt->second.json_double_value.end(); ++it) {
        json_double_value[it->first] = it->second;
      }

      o[paramIt->first] = json_double_value;
    } else if (!paramIt->second.string_value.empty()) {
      SerializeStringProperty(paramIt->first, paramIt->second.string_value, o);
    } else {
      o[paramIt->first] = paramIt->second.bool_value;
    }
  }
}

static void SerializeGltfAccessor(Accessor &accessor, json &o) {
  SerializeNumberProperty<int>("bufferView", accessor.bufferView, o);

  if (accessor.byteOffset != 0.0)
    SerializeNumberProperty<int>("byteOffset", int(accessor.byteOffset), o);

  SerializeNumberProperty<int>("componentType", accessor.componentType, o);
  SerializeNumberProperty<size_t>("count", accessor.count, o);
  SerializeNumberArrayProperty<double>("min", accessor.minValues, o);
  SerializeNumberArrayProperty<double>("max", accessor.maxValues, o);
  std::string type;
  switch (accessor.type) {
    case TINYGLTF_TYPE_SCALAR:
      type = "SCALAR";
      break;
    case TINYGLTF_TYPE_VEC2:
      type = "VEC2";
      break;
    case TINYGLTF_TYPE_VEC3:
      type = "VEC3";
      break;
    case TINYGLTF_TYPE_VEC4:
      type = "VEC4";
      break;
    case TINYGLTF_TYPE_MAT2:
      type = "MAT2";
      break;
    case TINYGLTF_TYPE_MAT3:
      type = "MAT3";
      break;
    case TINYGLTF_TYPE_MAT4:
      type = "MAT4";
      break;
  }

  SerializeStringProperty("type", type, o);
}

static void SerializeGltfAnimationChannel(AnimationChannel &channel,
                                          json &o) {
  SerializeNumberProperty("sampler", channel.sampler, o);
  json target;
  SerializeNumberProperty("node", channel.target_node, target);
  SerializeStringProperty("path", channel.target_path, target);

  o["target"] = target;
}

static void SerializeGltfAnimationSampler(AnimationSampler &sampler,
                                          json &o) {
  SerializeNumberProperty("input", sampler.input, o);
  SerializeNumberProperty("output", sampler.output, o);
  SerializeStringProperty("interpolation", sampler.interpolation, o);
}

static void SerializeGltfAnimation(Animation &animation, json &o) {
  SerializeStringProperty("name", animation.name, o);
  json channels;
  for (unsigned int i = 0; i < animation.channels.size(); ++i) {
    json channel;
    AnimationChannel gltfChannel = animation.channels[i];
    SerializeGltfAnimationChannel(gltfChannel, channel);
    channels.push_back(channel);
  }
  o["channels"] = channels;

  json samplers;
  for (unsigned int i = 0; i < animation.samplers.size(); ++i) {
    json sampler;
    AnimationSampler gltfSampler = animation.samplers[i];
    SerializeGltfAnimationSampler(gltfSampler, sampler);
    samplers.push_back(sampler);
  }

  o["samplers"] = samplers;
}

static void SerializeGltfAsset(Asset &asset, json &o) {
  if (!asset.generator.empty()) {
    SerializeStringProperty("generator", asset.generator, o);
  }

  if (!asset.version.empty()) {
    SerializeStringProperty("version", asset.version, o);
  }

  if (asset.extras.Keys().size()) {
    SerializeValue("extras", asset.extras, o);
  }
}

static void SerializeGltfBuffer(Buffer &buffer, json &o,
                                const std::string &binFilePath) {
  SerializeGltfBufferData(buffer.data, binFilePath);
  SerializeNumberProperty("byteLength", buffer.data.size(), o);
  SerializeStringProperty("uri", binFilePath, o);

  if (buffer.name.size()) SerializeStringProperty("name", buffer.name, o);
}

static void SerializeGltfBufferView(BufferView &bufferView,
                                    json &o) {
  SerializeNumberProperty("buffer", bufferView.buffer, o);
  SerializeNumberProperty<size_t>("byteLength", bufferView.byteLength, o);
  SerializeNumberProperty<size_t>("byteStride", bufferView.byteStride, o);
  SerializeNumberProperty<size_t>("byteOffset", bufferView.byteOffset, o);
  SerializeNumberProperty("target", bufferView.target, o);

  if (bufferView.name.size()) {
    SerializeStringProperty("name", bufferView.name, o);
  }
}

// Only external textures are serialized for now
static void SerializeGltfImage(Image &image, json &o) {
  SerializeStringProperty("uri", image.uri, o);

  if (image.name.size()) {
    SerializeStringProperty("name", image.name, o);
  }
}

static void SerializeGltfMaterial(Material &material, json &o) {
  if (material.extPBRValues.size()) {
    // Serialize PBR specular/glossiness material
    json values;
    SerializeParameterMap(material.extPBRValues, values);

    json extension;
    o["extensions"] = extension;
  }

  if (material.values.size()) {
    json pbrMetallicRoughness;
    SerializeParameterMap(material.values, pbrMetallicRoughness);
    o["pbrMetallicRoughness"] = pbrMetallicRoughness;
  }

  json additionalValues;
  SerializeParameterMap(material.additionalValues, o);

  if (material.name.size()) {
    SerializeStringProperty("name", material.name, o);
  }
}

static void SerializeGltfMesh(Mesh &mesh, json &o) {
  json primitives;
  for (unsigned int i = 0; i < mesh.primitives.size(); ++i) {
    json primitive;
    json attributes;
    Primitive gltfPrimitive = mesh.primitives[i];
    for (std::map<std::string, int>::iterator attrIt =
             gltfPrimitive.attributes.begin();
         attrIt != gltfPrimitive.attributes.end(); ++attrIt) {
      SerializeNumberProperty<int>(attrIt->first, attrIt->second, attributes);
    }

    primitive["attributes"] = attributes;
    SerializeNumberProperty<int>("indices", gltfPrimitive.indices, primitive);
    SerializeNumberProperty<int>("material", gltfPrimitive.material, primitive);
    SerializeNumberProperty<int>("mode", gltfPrimitive.mode, primitive);

    // Morph targets
    if (gltfPrimitive.targets.size()) {
      json targets;
      for (unsigned int k = 0; k < gltfPrimitive.targets.size(); ++k) {
        json targetAttributes;
        std::map<std::string, int> targetData = gltfPrimitive.targets[k];
        for (std::map<std::string, int>::iterator attrIt = targetData.begin();
             attrIt != targetData.end(); ++attrIt) {
          SerializeNumberProperty<int>(attrIt->first, attrIt->second,
                                       targetAttributes);
        }

        targets.push_back(targetAttributes);
      }
      primitive["targets"] = targets;
    }

    primitives.push_back(primitive);
  }

  o["primitives"] = primitives;
  if (mesh.weights.size()) {
    SerializeNumberArrayProperty<double>("weights", mesh.weights, o);
  }

  if (mesh.name.size()) {
    SerializeStringProperty("name", mesh.name, o);
  }
}

static void SerializeGltfLight(Light &light, json &o) {
  SerializeStringProperty("name", light.name, o);
  SerializeNumberArrayProperty("color", light.color, o);
  SerializeStringProperty("type", light.type, o);
}

static void SerializeGltfNode(Node &node, json &o) {
  if (node.translation.size() > 0) {
    SerializeNumberArrayProperty<double>("translation", node.translation, o);
  }
  if (node.rotation.size() > 0) {
    SerializeNumberArrayProperty<double>("rotation", node.rotation, o);
  }
  if (node.scale.size() > 0) {
    SerializeNumberArrayProperty<double>("scale", node.scale, o);
  }
  if (node.matrix.size() > 0) {
    SerializeNumberArrayProperty<double>("matrix", node.matrix, o);
  }
  if (node.mesh != -1) {
    SerializeNumberProperty<int>("mesh", node.mesh, o);
  }

  if (node.skin != -1) {
    SerializeNumberProperty<int>("skin", node.skin, o);
  }

  if (node.camera != -1) {
    SerializeNumberProperty<int>("camera", node.camera, o);
  }

  if (node.extLightsValues.size()) {
    json values;
    SerializeParameterMap(node.extLightsValues, values);
    json lightsExt;
    lightsExt["KHR_lights_cmn"] = values;
    o["extensions"] = lightsExt;
  }


  SerializeStringProperty("name", node.name, o);
  SerializeNumberArrayProperty<int>("children", node.children, o);
}

static void SerializeGltfSampler(Sampler &sampler, json &o) {
  SerializeNumberProperty("magFilter", sampler.magFilter, o);
  SerializeNumberProperty("minFilter", sampler.minFilter, o);
  SerializeNumberProperty("wrapS", sampler.wrapS, o);
  SerializeNumberProperty("wrapT", sampler.wrapT, o);
}

static void SerializeGltfOrthographicCamera(const OrthographicCamera &camera,
                                            json &o) {
  SerializeNumberProperty("zfar", camera.zfar, o);
  SerializeNumberProperty("znear", camera.znear, o);
  SerializeNumberProperty("xmag", camera.xmag, o);
  SerializeNumberProperty("ymag", camera.ymag, o);
}

static void SerializeGltfPerspectiveCamera(const PerspectiveCamera &camera,
                                           json &o) {
  SerializeNumberProperty("zfar", camera.zfar, o);
  SerializeNumberProperty("znear", camera.znear, o);
  if (camera.aspectRatio > 0) {
    SerializeNumberProperty("aspectRatio", camera.aspectRatio, o);
  }

  if (camera.yfov > 0) {
    SerializeNumberProperty("yfov", camera.yfov, o);
  }
}

static void SerializeGltfCamera(const Camera &camera, json &o) {
  SerializeStringProperty("type", camera.type, o);
  if (!camera.name.empty()) {
    SerializeStringProperty("name", camera.type, o);
  }

  if (camera.type.compare("orthographic") == 0) {
    json orthographic;
    SerializeGltfOrthographicCamera(camera.orthographic, orthographic);
    o["orthographic"] = orthographic;
  } else if (camera.type.compare("perspective") == 0) {
    json perspective;
    SerializeGltfPerspectiveCamera(camera.perspective, perspective);
    o["perspective"] = perspective;
  } else {
    // ???
  }
}

static void SerializeGltfScene(Scene &scene, json &o) {
  SerializeNumberArrayProperty<int>("nodes", scene.nodes, o);

  if (scene.name.size()) {
    SerializeStringProperty("name", scene.name, o);
  }
}

static void SerializeGltfSkin(Skin &skin, json &o) {
  if (skin.inverseBindMatrices != -1)
    SerializeNumberProperty("inverseBindMatrices", skin.inverseBindMatrices, o);

  SerializeNumberArrayProperty<int>("joints", skin.joints, o);
  SerializeNumberProperty("skeleton", skin.skeleton, o);
  if (skin.name.size()) {
    SerializeStringProperty("name", skin.name, o);
  }
}

static void SerializeGltfTexture(Texture &texture, json &o) {
  SerializeNumberProperty("sampler", texture.sampler, o);
  SerializeNumberProperty("source", texture.source, o);

  if (texture.extras.Size()) {
    json extras;
    SerializeValue("extras", texture.extras, o);
    o["extras"] = extras;
  }
}

static void WriteGltfFile(const std::string &output,
                          const std::string &content) {
  std::ofstream gltfFile(output.c_str());
  gltfFile << content << std::endl;
}

bool TinyGLTF::WriteGltfSceneToFile(
    Model *model,
    const std::string
        &filename /*, bool embedImages, bool embedBuffers, bool writeBinary*/) {
  json output;

  // ACCESSORS
  json accessors;
  for (unsigned int i = 0; i < model->accessors.size(); ++i) {
    json accessor;
    SerializeGltfAccessor(model->accessors[i], accessor);
    accessors.push_back(accessor);
  }
  output["accessors"] = accessors;

  // ANIMATIONS
  if (model->animations.size()) {
    json animations;
    for (unsigned int i = 0; i < model->animations.size(); ++i) {
      if (model->animations[i].channels.size()) {
        json animation;
        SerializeGltfAnimation(model->animations[i], animation);
        animations.push_back(animation);
      }
    }
    output["animations"] = animations;
  }

  // ASSET
  json asset;
  SerializeGltfAsset(model->asset, asset);
  output["asset"] = asset;

  std::string binFilePath = filename;
  std::string ext = ".bin";
  std::string::size_type pos = binFilePath.rfind('.', binFilePath.length());

  if (pos != std::string::npos) {
    binFilePath = binFilePath.substr(0, pos) + ext;
  } else {
    binFilePath = "./" + binFilePath + ".bin";
  }

  // BUFFERS (We expect only one buffer here)
  json buffers;
  for (unsigned int i = 0; i < model->buffers.size(); ++i) {
    json buffer;
    SerializeGltfBuffer(model->buffers[i], buffer, binFilePath);
    buffers.push_back(buffer);
  }
  output["buffers"] = buffers;

  // BUFFERVIEWS
  json bufferViews;
  for (unsigned int i = 0; i < model->bufferViews.size(); ++i) {
    json bufferView;
    SerializeGltfBufferView(model->bufferViews[i], bufferView);
    bufferViews.push_back(bufferView);
  }
  output["bufferViews"] = bufferViews;

  // Extensions used
  if (model->extensionsUsed.size()) {
    SerializeStringArrayProperty("extensionsUsed", model->extensionsUsed,
                                 output);
  }

  // Extensions required
  if (model->extensionsRequired.size()) {
    SerializeStringArrayProperty("extensionsRequired",
                                 model->extensionsRequired, output);
  }

  // IMAGES
  json images;
  for (unsigned int i = 0; i < model->images.size(); ++i) {
    json image;
    SerializeGltfImage(model->images[i], image);
    images.push_back(image);
  }
  output["images"] = images;

  // MATERIALS
  json materials;
  for (unsigned int i = 0; i < model->materials.size(); ++i) {
    json material;
    SerializeGltfMaterial(model->materials[i], material);
    materials.push_back(material);
  }
  output["materials"] = materials;

  // MESHES
  json meshes;
  for (unsigned int i = 0; i < model->meshes.size(); ++i) {
    json mesh;
    SerializeGltfMesh(model->meshes[i], mesh);
    meshes.push_back(mesh);
  }
  output["meshes"] = meshes;

  // NODES
  json nodes;
  for (unsigned int i = 0; i < model->nodes.size(); ++i) {
    json node;
    SerializeGltfNode(model->nodes[i], node);
    nodes.push_back(node);
  }
  output["nodes"] = nodes;

  // SCENE
  SerializeNumberProperty<int>("scene", model->defaultScene, output);

  // SCENES
  json scenes;
  for (unsigned int i = 0; i < model->scenes.size(); ++i) {
    json currentScene;
    SerializeGltfScene(model->scenes[i], currentScene);
    scenes.push_back(currentScene);
  }
  output["scenes"] = scenes;

  // SKINS
  if (model->skins.size()) {
    json skins;
    for (unsigned int i = 0; i < model->skins.size(); ++i) {
      json skin;
      SerializeGltfSkin(model->skins[i], skin);
      skins.push_back(skin);
    }
    output["skins"] = skins;
  }

  // TEXTURES
  json textures;
  for (unsigned int i = 0; i < model->textures.size(); ++i) {
    json texture;
    SerializeGltfTexture(model->textures[i], texture);
    textures.push_back(texture);
  }
  output["textures"] = textures;

  // SAMPLERS
  json samplers;
  for (unsigned int i = 0; i < model->samplers.size(); ++i) {
    json sampler;
    SerializeGltfSampler(model->samplers[i], sampler);
    samplers.push_back(sampler);
  }
  output["samplers"] = samplers;

  // CAMERAS
  json cameras;
  for (unsigned int i = 0; i < model->cameras.size(); ++i) {
    json camera;
    SerializeGltfCamera(model->cameras[i], camera);
    cameras.push_back(camera);
  }
  output["cameras"] = cameras;

  // LIGHTS
  json lights;
  for (unsigned int i = 0; i < model->lights.size(); ++i) {
    json light;
    SerializeGltfLight(model->lights[i], light);
    lights.push_back(light);
  }
  output["lights"] = lights;

  WriteGltfFile(filename, output.dump());
  return true;
}

}  // namespace tinygltf

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif  // TINYGLTF_IMPLEMENTATION
