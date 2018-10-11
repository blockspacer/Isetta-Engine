/*
 * Copyright (c) 2018 Isetta
 */
#pragma once
#include <string>
#include "Core/Color.h"
#include "Graphics/RenderNode.h"
#include "Scene/Component.h"
#include "Scene/Entity.h"

namespace Isetta {
class LightComponent : public Component {
 public:
  enum class Property {
    RADIUS,
    FOV,
    SHADOW_MAP_COUNT,
    SHADOW_MAP_BIAS,
    COLOR,
    COLOR_MULTIPLIER
  };

  void OnEnable() override;
  void OnDisable() override;

  LightComponent(std::string resourceName, std::string lightName);

  template <Property Attr, typename T>
  void SetProperty(T value);

  template <Property Attr, typename T>
  T GetProperty() const;

 private:
  H3DRes LoadResourceFromFile(std::string resourceName);

  static class RenderModule* renderModule;
  friend class RenderModule;
  void UpdateTransform();
  std::string name;
  H3DNode renderNode;
  H3DRes renderResource;
};

template <LightComponent::Property Attr, typename T>
void LightComponent::SetProperty(T value) {
  if constexpr (Attr == Property::RADIUS) {
    h3dSetNodeParamF(renderNode, H3DLight::RadiusF, 0, value);
  } else if constexpr (Attr == Property::FOV) {
    h3dSetNodeParamF(renderNode, H3DLight::FovF, 0, value);
  } else if constexpr (Attr == Property::SHADOW_MAP_COUNT) {
    h3dSetNodeParamI(renderNode, H3DLight::ShadowMapCountI, value);
  } else if constexpr (Attr == Property::SHADOW_MAP_BIAS) {
    h3dSetNodeParamF(renderNode, H3DLight::ShadowMapBiasF, 0, value);
  } else if constexpr (Attr == Property::COLOR) {
    h3dSetNodeParamF(renderNode, H3DLight::ColorF3, 0, value.r);
    h3dSetNodeParamF(renderNode, H3DLight::ColorF3, 1, value.g);
    h3dSetNodeParamF(renderNode, H3DLight::ColorF3, 2, value.b);
  } else if constexpr (Attr == Property::COLOR_MULTIPLIER) {
    h3dSetNodeParamF(renderNode, H3DLight::ColorMultiplierF, 0, value);
  }
}

template <LightComponent::Property Attr, typename T>
T LightComponent::GetProperty() const {
  if constexpr (Attr == Property::RADIUS) {
    return h3dGetNodeParamF(renderNode, H3DLight::RadiusF, 0);
  } else if constexpr (Attr == Property::FOV) {
    return h3dGetNodeParamF(renderNode, H3DLight::FovF, 0);
  } else if constexpr (Attr == Property::SHADOW_MAP_COUNT) {
    return h3dGetNodeParamI(renderNode, H3DLight::ShadowMapCountI);
  } else if constexpr (Attr == Property::SHADOW_MAP_BIAS) {
    return h3dGetNodeParamF(renderNode, H3DLight::ShadowMapBiasF, 0);
  } else if constexpr (Attr == Property::COLOR) {
    Color c;
    c.r = h3dGetNodeParamF(renderNode, H3DLight::ColorF3, 0);
    c.g = h3dGetNodeParamF(renderNode, H3DLight::ColorF3, 1);
    c.b = h3dGetNodeParamF(renderNode, H3DLight::ColorF3, 2);
    return c;
  } else if constexpr (Attr == Property::COLOR_MULTIPLIER) {
    return h3dGetNodeParamF(renderNode, H3DLight::ColorMultiplierF, 0);
  }
}
}  // namespace Isetta