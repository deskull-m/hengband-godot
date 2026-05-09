#pragma once

#include <tl/optional.hpp>

class ArtifactType;
class BaseitemKey;
class ItemEntity;
class ArtifactService {
public:
    ArtifactService() = delete;

    static tl::optional<ItemEntity> try_make_instant_artifact(int making_level);

private:
    static tl::optional<BaseitemKey> try_make_instant_artifact(const ArtifactType &artifact, int making_level);
    static bool can_make_instant_artifact(const ArtifactType &artifact);
    static bool evaluate_shallow_instant_artifact(const ArtifactType &artifact, int making_level);
    static bool evaluate_rarity(const ArtifactType &artifact);
    static bool evaluate_shallow_baseitem(const ArtifactType &artifact, int making_level);
};
