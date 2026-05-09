#pragma once

#include "util/abstract-map-wrapper.h"

enum class FixedArtifactId : short;
class ArtifactType;
class ItemEntity;
class ArtifactList : public util::AbstractMapWrapper<FixedArtifactId, ArtifactType> {
public:
    ArtifactList(const ArtifactList &) = delete;
    ArtifactList(ArtifactList &&) = delete;
    ArtifactList &operator=(const ArtifactList &) = delete;
    ArtifactList &operator=(ArtifactList &&) = delete;
    ~ArtifactList() = default;

    static ArtifactList &get_instance();
    const ArtifactType &get_artifact(const FixedArtifactId fa_id) const;
    ArtifactType &get_artifact(const FixedArtifactId fa_id);

    bool order(const FixedArtifactId id1, const FixedArtifactId id2) const;
    void emplace(const FixedArtifactId fa_id, ArtifactType &&artifact);

private:
    ArtifactList() = default;
    static ArtifactList instance;
    static ArtifactType dummy;

    std::map<FixedArtifactId, ArtifactType> artifacts{};

    std::map<FixedArtifactId, ArtifactType> &get_inner_container() override
    {
        return this->artifacts;
    }
};
