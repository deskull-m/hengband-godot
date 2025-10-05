/*!
 * @brief 一部職業でのみダンプする能力の出力処理
 * @date 2020/03/07
 * @author Hourier
 */

#include "io-dump/special-class-dump.h"
#include "blue-magic/blue-magic-checker.h"
#include "cmd-item/cmd-magiceat.h"
#include "mind/mind-blue-mage.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/monster-power-table.h"
#include "object/tval-types.h"
#include "player-base/player-class.h"
#include "player-info/bluemage-data.h"
#include "player-info/magic-eater-data-type.h"
#include "smith/object-smith.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"
#include "util/flag-group.h"
#include <algorithm>
#include <fmt/format.h>
#include <iterator>
#include <string>
#include <vector>

struct learnt_spell_table {
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
};

/*!
 * @brief 魔力喰いを持つクラスの情報をダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_magic_eater(PlayerType *player_ptr, FILE *fff)
{
    auto magic_eater_data = PlayerClass(player_ptr).get_specific_data<MagicEaterDataList>();
    if (!magic_eater_data) {
        return;
    }

    fprintf(fff, _("\n\n  [取り込んだ魔法道具]\n", "\n\n  [Magic devices eaten]\n"));
    const auto &baseitems = BaseitemList::get_instance();
    for (auto tval : { ItemKindType::STAFF, ItemKindType::WAND, ItemKindType::ROD }) {
        switch (tval) {
        case ItemKindType::STAFF:
            fprintf(fff, _("\n[杖]\n", "\n[Staffs]\n"));
            break;
        case ItemKindType::WAND:
            fprintf(fff, _("\n[魔法棒]\n", "\n[Wands]\n"));
            break;
        case ItemKindType::ROD:
            fprintf(fff, _("\n[ロッド]\n", "\n[Rods]\n"));
            break;
        default:
            break;
        }

        const auto &item_group = magic_eater_data->get_item_group(tval);
        std::vector<std::string> desc_list;
        for (auto i = 0U; i < item_group.size(); ++i) {
            auto &item = item_group[i];
            if (item.count == 0) {
                continue;
            }

            const auto &baseitem = baseitems.lookup_baseitem({ tval, i });
            const auto buf = format("%23s (%2d)", baseitem.name.data(), item.count);
            desc_list.emplace_back(buf);
        }

        if (desc_list.size() <= 0) {
            fputs(_("  (なし)\n", "  (none)\n"), fff);
            continue;
        }

        uint i;
        for (i = 0; i < desc_list.size(); i++) {
            fputs(desc_list[i].data(), fff);
            if (i % 3 < 2) {
                fputs("    ", fff);
            } else {
                fputs("\n", fff);
            }
        }

        if (i % 3 > 0) {
            fputs("\n", fff);
        }
    }
}

/*!
 * @brief 鍛冶師のエッセンス情報をダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_smith(PlayerType *player_ptr, FILE *fff)
{
    fprintf(fff, _("\n\n  [手に入れたエッセンス]\n\n", "\n\n  [Get Essence]\n\n"));
    fprintf(fff, _("エッセンス   個数     エッセンス   個数     エッセンス   個数", "Essence      Num      Essence      Num      Essence      Num "));

    const auto &essences = Smith::get_essence_list();
    auto n = essences.size();
    std::vector<int> amounts;
    std::transform(essences.begin(), essences.end(), std::back_inserter(amounts),
        [smith = Smith(player_ptr)](SmithEssenceType e) { return smith.get_essence_num_of_posessions(e); });

    auto row = n / 3 + 1;
    for (auto i = 0U; i < row; i++) {
        fprintf(fff, "\n");
        fprintf(fff, "%-11s %5d     ", Smith::get_essence_name(essences[i]), amounts[i]);
        if (i + row < n) {
            fprintf(fff, "%-11s %5d     ", Smith::get_essence_name(essences[i + row]), amounts[i + row]);
        }
        if (i + row * 2 < n) {
            fprintf(fff, "%-11s %5d", Smith::get_essence_name(essences[i + row * 2]), amounts[i + row * 2]);
        }
    }

    fputs("\n", fff);
}

/*!
 * @brief ダンプする情報に学習済魔法の種類を追加する
 * @param col 行数
 * @param blue_magic_type 魔法の種類
 * @param learnt_spell_ptr 学習済魔法のテーブル
 * @return ダンプ用情報
 */
static std::string add_monster_spell_type(BlueMagicType blue_magic_type, learnt_spell_table &learnt_spell_ptr)
{
    learnt_spell_ptr.ability_flags.clear();
    set_rf_masks(learnt_spell_ptr.ability_flags, blue_magic_type);
    switch (blue_magic_type) {
    case BlueMagicType::BOLT:
        return _("\n     [ボルト型]\n", "\n     [Bolt  Type]\n");
    case BlueMagicType::BALL:
        return _("\n     [ボール型]\n", "\n     [Ball  Type]\n");
    case BlueMagicType::BREATH:
        return _("\n     [ブレス型]\n", "\n     [  Breath  ]\n");
    case BlueMagicType::SUMMON:
        return _("\n     [召喚魔法]\n", "\n     [Summonning]\n");
    case BlueMagicType::OTHER:
        return _("\n     [ その他 ]\n", "\n     [Other Type]\n");
    default:
        return "";
    }
}

/*!
 * @brief 青魔道士の学習済魔法をダンプする
 * @param bluemage_data 青魔道士の固有データへの参照
 * @return ダンプ用情報の行リスト
 */
static std::vector<std::string> build_learnt_magics_info(bluemage_data_type &bluemage_data)
{
    std::vector<std::string> lines;
    lines.emplace_back(_("\n\n  [学習済みの青魔法]\n", "\n\n  [Learned Blue Magic]\n"));
    for (auto blue_magic_type : BLUE_MAGIC_TYPE_LIST) {
        lines.emplace_back("");
        learnt_spell_table learnt_magic;
        lines.push_back(add_monster_spell_type(blue_magic_type, learnt_magic));
        learnt_magic.ability_flags &= bluemage_data.learnt_blue_magics;

        std::vector<MonsterAbilityType> learnt_spells;
        EnumClassFlagGroup<MonsterAbilityType>::get_flags(learnt_magic.ability_flags, std::back_inserter(learnt_spells));
        lines.emplace_back("");
        lines.emplace_back("       ");
        for (const auto spell : learnt_spells) {
            const auto l1 = lines.cend()->length();
            const auto l2 = monster_powers_short.at(spell).length();
            if ((l1 + l2) >= 75) {
                lines.emplace_back("\n");
                lines.emplace_back("");
                lines.emplace_back("       ");
            }

            lines.emplace_back(monster_powers_short.at(spell));
            lines.emplace_back(", ");
        }

        if (learnt_spells.empty()) {
            lines.emplace_back(_("なし", "None"));
            lines.emplace_back("\n");
            continue;
        }

        lines.emplace_back("\n");
    }

    return lines;
}

/*!
 * @brief プレイヤーの職業能力情報をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
void dump_aux_class_special(PlayerType *player_ptr, FILE *fff)
{
    switch (player_ptr->pclass) {
    case PlayerClassType::MAGIC_EATER: {
        dump_magic_eater(player_ptr, fff);
        return;
    }
    case PlayerClassType::SMITH: {
        dump_smith(player_ptr, fff);
        return;
    }
    case PlayerClassType::BLUE_MAGE: {
        const auto bluemage_data = PlayerClass(player_ptr).get_specific_data<bluemage_data_type>();
        std::vector<std::string> lines;
        if (bluemage_data) {
            lines = build_learnt_magics_info(*bluemage_data);
        }

        for (const auto &line : lines) {
            fmt::print(fff, "{}", line);
        }

        return;
    }
    default:
        return;
    }
}
