#include "system/artifact/artifact-record.h"

bool ArtifactRecord::get_generated() const
{
    return this->is_generated;
}

bool ArtifactRecord::get_identified() const
{
    return this->is_identified;
}

bool ArtifactRecord::get_known() const
{
    return this->is_known;
}

void ArtifactRecord::set_generated(bool new_state)
{
    this->is_generated = new_state;
}

void ArtifactRecord::set_identified(bool new_state)
{
    this->is_identified = new_state;
}

void ArtifactRecord::set_known(bool new_state)
{
    this->is_known = new_state;
}

bool ArtifactRecords::get_generated(FixedArtifactId fa_id) const
{
    return this->records.at(fa_id).get_generated();
}

bool ArtifactRecords::get_identified(FixedArtifactId fa_id) const
{
    return this->records.at(fa_id).get_identified();
}

bool ArtifactRecords::get_known(FixedArtifactId fa_id) const
{
    return this->records.at(fa_id).get_known();
}

void ArtifactRecords::set_generated(FixedArtifactId fa_id, bool new_state)
{
    return this->records.at(fa_id).set_generated(new_state);
}

void ArtifactRecords::set_identified(FixedArtifactId fa_id, bool new_state)
{
    return this->records.at(fa_id).set_identified(new_state);
}

void ArtifactRecords::set_known(FixedArtifactId fa_id, bool new_state)
{
    return this->records.at(fa_id).set_known(new_state);
}

void ArtifactRecords::reset_all_without_knowledge()
{
    for (auto &[_, record] : this->records) {
        record.set_generated(false);
        record.set_identified(false);
    }
}
