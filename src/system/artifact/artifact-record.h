#pragma once

#include "util/abstract-map-wrapper.h"

class ArtifactRecord {
public:
    ArtifactRecord() = default;

    bool get_generated() const;
    bool get_identified() const;
    bool get_known() const;

    void set_generated(bool new_state);
    void set_identified(bool new_state = true); //!< ニューゲーム時またはウィザードモードでのみfalseをsetする.
    void set_known(bool new_state = true); //!< ウィザードモードでのみfalseをsetする.

private:
    bool is_generated = false; //!< 生成済か否か (生成済でも、「保存モードON」かつ「帰還等で鑑定前に消滅」したら未生成状態に戻る)
    bool is_identified = false; //!< 鑑定 or *鑑定*済か否か (簡易鑑定の段階ではfalse)
    bool is_known = false; //!< 今までに鑑定したり、噂を聞いたことがあるか (前世のセーブも含)
};

enum class FixedArtifactId;
class ArtifactRecord;
class ArtifactRecords : public util::AbstractMapWrapper<FixedArtifactId, ArtifactRecord> {
    ArtifactRecords(ArtifactRecords &&) = delete;
    ArtifactRecords(const ArtifactRecords &) = delete;
    ArtifactRecords &operator=(const ArtifactRecords &) = delete;
    ArtifactRecords &operator=(ArtifactRecords &&) = delete;
    ~ArtifactRecords() = default;

    static ArtifactRecords &get_instance();
    bool get_generated(FixedArtifactId fa_id) const;
    bool get_identified(FixedArtifactId fa_id) const;
    bool get_known(FixedArtifactId fa_id) const;

    void set_generated(FixedArtifactId fa_id, bool new_state);
    void set_identified(FixedArtifactId fa_id, bool new_state = true);
    void set_known(FixedArtifactId fa_id, bool new_state = true);
    void reset_all_without_knowledge();

private:
    ArtifactRecords();
    static ArtifactRecords instance;
    std::map<FixedArtifactId, ArtifactRecord> records;

    std::map<FixedArtifactId, ArtifactRecord> &get_inner_container() override
    {
        return this->records;
    }
};
