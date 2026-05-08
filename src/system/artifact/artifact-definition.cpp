#include "system/artifact/artifact-definition.h"
#include "artifact/fixed-art-types.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item-entity.h"

ArtifactType::ArtifactType()
    : bi_key(BaseitemKey(ItemKindType::NONE))
{
}

/*!
 * @brief アーティファクトが生成可能か否かを確認する
 * @param bi_key 生成しようとするアーティファクトのベースアイテムキー
 * @param level プレイヤーが今いる階層
 */
bool ArtifactType::can_generate(const BaseitemKey &generaing_bi_key) const
{
    if (this->is_generated) {
        return false;
    }

    if (this->gen_flags.has(ItemGenerationTraitType::QUESTITEM)) {
        return false;
    }

    if (this->gen_flags.has(ItemGenerationTraitType::INSTA_ART)) {
        return false;
    }

    return this->bi_key == generaing_bi_key;
}

/*!
 * @brief INSTA_ART型の固定アーティファクト生成を試みる
 * @param 生成基準階層 (現在フロアそのものではなくボーナスつき)
 * @param fa_id 固定アーティファクトID
 * @return 生成に成功したらそのアイテム、失敗したらnullopt
 */
tl::optional<BaseitemKey> ArtifactType::try_make_instant_artifact(int making_level) const
{
    if (!this->can_make_instant_artifact()) {
        return tl::nullopt;
    }

    if (!this->evaluate_shallow_instant_artifact(making_level)) {
        return tl::nullopt;
    }

    if (!this->evaluate_rarity()) {
        return tl::nullopt;
    }

    if (!this->evaluate_shallow_baseitem(making_level)) {
        return tl::nullopt;
    }

    return this->bi_key;
}

/*!
 * @brief INSTA_ARTフラグ付きアーティファクトの生成可否を判定する
 * @return 生成可否
 * @details 生成済、クエスト属性付き、非INSTA_ARTはfalse、普通のINSTA_ARTはtrue
 */
bool ArtifactType::can_make_instant_artifact() const
{
    auto can_make = !this->is_generated;
    can_make &= this->gen_flags.has_not(ItemGenerationTraitType::QUESTITEM);
    can_make &= this->gen_flags.has(ItemGenerationTraitType::INSTA_ART);
    return can_make;
}

/*!
 * @brief 標準生成階層より浅い階層での生成制限を判定する
 * @return 生成可否
 * @details 1/(不足階層*2) を満たさないと生成しない
 */
bool ArtifactType::evaluate_shallow_instant_artifact(int making_level) const
{
    if (this->level <= making_level) {
        return true;
    }

    return one_in_((this->level - making_level) * 2);
}

/*!
 * @brief レアリティによる生成制限を判定する
 * @return 生成可否
 */
bool ArtifactType::evaluate_rarity() const
{
    return one_in_(this->rarity);
}

/*!
 * @brief 標準生成階層より浅い階層でのベースアイテム生成制限を判定する
 * @return 生成可否
 * @details 1/(不足階層*5) を満たさないと生成しない
 */
bool ArtifactType::evaluate_shallow_baseitem(int making_level) const
{
    const auto &baseitems = BaseitemList::get_instance();
    const auto &baseitem = baseitems.lookup_baseitem(this->bi_key);
    if (baseitem.level <= making_level) {
        return true;
    }

    return one_in_((baseitem.level - making_level) * 5);
}
