/*!
 * @brief 自動拾いの記述
 * @date 2020/04/25
 * @author Hourier
 * @todo 1つ300行近い関数なので後ほど要分割
 */

#include "autopick/autopick-describer.h"
#include "autopick/autopick-flags-table.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "system/angband.h"
#include "util/string-processor.h"
#include <string>
#include <vector>

struct autopick_describer {
    concptr str;
    EnumClassFlagGroup<AutopickMethod> act;
    concptr insc;
    bool top;
    concptr body_str;
};

#if JP
static void describe_autpick_jp(char *buff, const autopick_type &entry, autopick_describer *describer)
{
    std::vector<std::string> before_strings;
    before_strings.reserve(100);
    if (entry.has(FLG_COLLECTING)) {
        before_strings.emplace_back("収集中で既に持っているスロットにまとめられる");
    }

    if (entry.has(FLG_UNAWARE)) {
        before_strings.emplace_back("未鑑定でその効果も判明していない");
    }

    if (entry.has(FLG_UNIDENTIFIED)) {
        before_strings.emplace_back("未鑑定の");
    }

    if (entry.has(FLG_IDENTIFIED)) {
        before_strings.emplace_back("鑑定済みの");
    }

    if (entry.has(FLG_STAR_IDENTIFIED)) {
        before_strings.emplace_back("完全に鑑定済みの");
    }

    if (entry.has(FLG_BOOSTED)) {
        before_strings.emplace_back("ダメージダイスが通常より大きい");
        describer->body_str = "武器";
    }

    if (entry.has(FLG_MORE_DICE)) {
        describer->body_str = "武器";
        before_strings.emplace_back("ダメージダイスの最大値が");
        before_strings.emplace_back(std::to_string(entry.dice));
        before_strings.emplace_back("以上の");
    }

    if (entry.has(FLG_MORE_BONUS)) {
        before_strings.emplace_back("修正値が(+");
        before_strings.emplace_back(std::to_string(entry.bonus));
        before_strings.emplace_back(")以上の");
    }

    if (entry.has(FLG_WORTHLESS)) {
        before_strings.emplace_back("店で無価値と判定される");
    }

    if (entry.has(FLG_ARTIFACT)) {
        before_strings.emplace_back("アーティファクトの");
        describer->body_str = "装備";
    }

    if (entry.has(FLG_EGO)) {
        before_strings.emplace_back("エゴアイテムの");
        describer->body_str = "装備";
    }

    if (entry.has(FLG_GOOD)) {
        before_strings.emplace_back("上質の");
        describer->body_str = "装備";
    }

    if (entry.has(FLG_NAMELESS)) {
        before_strings.emplace_back("エゴでもアーティファクトでもない");
        describer->body_str = "装備";
    }

    if (entry.has(FLG_AVERAGE)) {
        before_strings.emplace_back("並の");
        describer->body_str = "装備";
    }

    if (entry.has(FLG_RARE)) {
        before_strings.emplace_back("ドラゴン装備やカオス・ブレード等を含む珍しい");
        describer->body_str = "装備";
    }

    if (entry.has(FLG_COMMON)) {
        before_strings.emplace_back("ありふれた(ドラゴン装備やカオス・ブレード等の珍しい物ではない)");
        describer->body_str = "装備";
    }

    if (entry.has(FLG_WANTED)) {
        before_strings.emplace_back("ハンター事務所で賞金首とされている");
        describer->body_str = "死体や骨";
    }

    if (entry.has(FLG_HUMAN)) {
        before_strings.emplace_back("悪魔魔法で使うための人間やヒューマノイドの");
        describer->body_str = "死体や骨";
    }

    if (entry.has(FLG_UNIQUE)) {
        before_strings.emplace_back("ユニークモンスターの");
        describer->body_str = "死体や骨";
    }

    if (entry.has(FLG_UNREADABLE)) {
        before_strings.emplace_back("あなたが読めない領域の");
        describer->body_str = "魔法書";
    }

    if (entry.has(FLG_REALM1)) {
        before_strings.emplace_back("第一領域の");
        describer->body_str = "魔法書";
    }

    if (entry.has(FLG_REALM2)) {
        before_strings.emplace_back("第二領域の");
        describer->body_str = "魔法書";
    }

    if (entry.has(FLG_FIRST)) {
        before_strings.emplace_back("全4冊の内の1冊目の");
        describer->body_str = "魔法書";
    }

    if (entry.has(FLG_SECOND)) {
        before_strings.emplace_back("全4冊の内の2冊目の");
        describer->body_str = "魔法書";
    }

    if (entry.has(FLG_THIRD)) {
        before_strings.emplace_back("全4冊の内の3冊目の");
        describer->body_str = "魔法書";
    }

    if (entry.has(FLG_FOURTH)) {
        before_strings.emplace_back("全4冊の内の4冊目の");
        describer->body_str = "魔法書";
    }

    if (entry.has(FLG_ITEMS)) {
        ;
    } /* Nothing to do */
    else if (entry.has(FLG_WEAPONS)) {
        describer->body_str = "武器";
    } else if (entry.has(FLG_FAVORITE_WEAPONS)) {
        describer->body_str = "得意武器";
    } else if (entry.has(FLG_ARMORS)) {
        describer->body_str = "防具";
    } else if (entry.has(FLG_MISSILES)) {
        describer->body_str = "弾や矢やクロスボウの矢";
    } else if (entry.has(FLG_DEVICES)) {
        describer->body_str = "巻物や魔法棒や杖やロッド";
    } else if (entry.has(FLG_LIGHTS)) {
        describer->body_str = "光源用のアイテム";
    } else if (entry.has(FLG_JUNKS)) {
        describer->body_str = "折れた棒等のガラクタ";
    } else if (entry.has(FLG_CORPSES)) {
        describer->body_str = "死体や骨";
    } else if (entry.has(FLG_SPELLBOOKS)) {
        describer->body_str = "魔法書";
    } else if (entry.has(FLG_HAFTED)) {
        describer->body_str = "鈍器";
    } else if (entry.has(FLG_SHIELDS)) {
        describer->body_str = "盾";
    } else if (entry.has(FLG_BOWS)) {
        describer->body_str = "スリングや弓やクロスボウ";
    } else if (entry.has(FLG_RINGS)) {
        describer->body_str = "指輪";
    } else if (entry.has(FLG_AMULETS)) {
        describer->body_str = "アミュレット";
    } else if (entry.has(FLG_SUITS)) {
        describer->body_str = "鎧";
    } else if (entry.has(FLG_CLOAKS)) {
        describer->body_str = "クローク";
    } else if (entry.has(FLG_HELMS)) {
        describer->body_str = "ヘルメットや冠";
    } else if (entry.has(FLG_GLOVES)) {
        describer->body_str = "籠手";
    } else if (entry.has(FLG_BOOTS)) {
        describer->body_str = "ブーツ";
    }

    *buff = '\0';
    if (before_strings.empty()) {
        strcat(buff, "全ての");
    } else {
        for (const auto &before_string : before_strings) {
            strcat(buff, before_string.data());
        }
    }

    strcat(buff, describer->body_str);

    if (*describer->str) {
        if (*describer->str == '^') {
            describer->str++;
            describer->top = true;
        }

        strcat(buff, "で、名前が「");
        angband_strcat(buff, describer->str, (MAX_NLEN - MAX_INSCRIPTION));
        if (describer->top) {
            strcat(buff, "」で始まるもの");
        } else {
            strcat(buff, "」を含むもの");
        }
    }

    if (describer->insc) {
        char tmp[MAX_INSCRIPTION + 1] = "";
        angband_strcat(tmp, describer->insc, MAX_INSCRIPTION);
        angband_strcat(buff, format("に「%s」", tmp), MAX_INSCRIPTION + 6);

        if (str_find(describer->insc, "%%all")) {
            strcat(buff, "(%%allは全能力を表す英字の記号で置換)");
        } else if (str_find(describer->insc, "%all")) {
            strcat(buff, "(%allは全能力を表す記号で置換)");
        } else if (str_find(describer->insc, "%%")) {
            strcat(buff, "(%%は追加能力を表す英字の記号で置換)");
        } else if (str_find(describer->insc, "%")) {
            strcat(buff, "(%は追加能力を表す記号で置換)");
        }

        strcat(buff, "と刻んで");
    } else {
        strcat(buff, "を");
    }

    if (describer->act.has(AutopickMethod::NOT_AUTOPICK)) {
        strcat(buff, "放置する。");
    } else if (describer->act.has(AutopickMethod::AUTODESTROY)) {
        strcat(buff, "破壊する。");
    } else if (describer->act.has(AutopickMethod::QUERY_AUTOPICK)) {
        strcat(buff, "確認の後に拾う。");
    } else {
        strcat(buff, "拾う。");
    }

    if (describer->act.has(AutopickMethod::DISPLAY)) {
        if (describer->act.has(AutopickMethod::NOT_AUTOPICK)) {
            strcat(buff, "全体マップ('M')で'N'を押したときに表示する。");
        } else if (describer->act.has(AutopickMethod::AUTODESTROY)) {
            strcat(buff, "全体マップ('M')で'K'を押したときに表示する。");
        } else {
            strcat(buff, "全体マップ('M')で'M'を押したときに表示する。");
        }
    } else {
        strcat(buff, "全体マップには表示しない。");
    }
}
#else

void describe_autopick_en(char *buff, const autopick_type &entry, autopick_describer *describer)
{
    std::vector<std::string_view> before_strings;
    before_strings.reserve(20);
    concptr after_str[20]{};
    concptr which_str[20]{};
    concptr whose_str[20]{};
    concptr whose_arg_str[20]{};
    char arg_str[2][24]{};
    int after_n = 0, which_n = 0, whose_n = 0, arg_n = 0;
    if (entry.has(FLG_COLLECTING)) {
        which_str[which_n++] = "can be absorbed into an existing inventory list slot";
    }

    if (entry.has(FLG_UNAWARE)) {
        before_strings.emplace_back("unidentified");
        whose_str[whose_n] = "basic abilities are not known";
        whose_arg_str[whose_n] = "";
        ++whose_n;
    }

    if (entry.has(FLG_UNIDENTIFIED)) {
        before_strings.emplace_back("unidentified");
    }

    if (entry.has(FLG_IDENTIFIED)) {
        before_strings.emplace_back("identified");
    }

    if (entry.has(FLG_STAR_IDENTIFIED)) {
        before_strings.emplace_back("fully identified");
    }

    if (entry.has(FLG_RARE)) {
        before_strings.emplace_back("very rare");
        after_str[after_n++] = "such as Dragon armor, Blades of Chaos, etc.";
    }

    if (entry.has(FLG_COMMON)) {
        before_strings.emplace_back("relatively common");
        after_str[after_n++] = "compared to very rare Dragon armor, Blades of Chaos, etc.";
    }

    if (entry.has(FLG_WORTHLESS)) {
        before_strings.emplace_back("worthless");
        which_str[which_n++] = "can not be sold at stores";
    }

    if (entry.has(FLG_ARTIFACT)) {
        before_strings.emplace_back("artifact");
    }

    if (entry.has(FLG_EGO)) {
        before_strings.emplace_back("ego");
    }

    if (entry.has(FLG_GOOD)) {
        which_str[which_n++] = "are of good quality";
    }

    if (entry.has(FLG_NAMELESS)) {
        which_str[which_n++] = "are neither ego items nor artifacts";
    }

    if (entry.has(FLG_AVERAGE)) {
        which_str[which_n++] = "are of average quality";
    }

    if (entry.has(FLG_BOOSTED)) {
        describer->body_str = "weapons";
        whose_str[whose_n] = "damage dice is bigger than normal";
        whose_arg_str[whose_n] = "";
        ++whose_n;
    }

    if (entry.has(FLG_MORE_DICE)) {
        describer->body_str = "weapons";
        whose_str[whose_n] = "maximum damage from dice is bigger than ";
        if (arg_n < (int)(sizeof(arg_str) / sizeof(arg_str[0]))) {
            snprintf(arg_str[arg_n], sizeof(arg_str[arg_n]), "%d", entry.dice);
            whose_arg_str[whose_n] = arg_str[arg_n];
            ++arg_n;
        } else {
            whose_arg_str[whose_n] = "garbled";
        }
        ++whose_n;
    }

    if (entry.has(FLG_MORE_BONUS)) {
        whose_str[whose_n] = "magical bonuses are bigger than (+";
        if (arg_n < (int)(sizeof(arg_str) / sizeof(arg_str[0]))) {
            snprintf(arg_str[arg_n], sizeof(arg_str[arg_n]), "%d)", entry.bonus);
            whose_arg_str[whose_n] = arg_str[arg_n];
            ++arg_n;
        } else {
            whose_arg_str[whose_n] = "garbled)";
        }
        ++whose_n;
    }

    if (entry.has(FLG_WANTED)) {
        describer->body_str = "corpses or skeletons";
        which_str[which_n++] = "are wanted at the Hunter's Office";
    }

    if (entry.has(FLG_HUMAN)) {
        before_strings.emplace_back("humanoid");
        describer->body_str = "corpses or skeletons";
        which_str[which_n++] = "can be used for Daemon magic";
    }

    if (entry.has(FLG_UNIQUE)) {
        before_strings.emplace_back("unique monsters'");
        describer->body_str = "corpses or skeletons";
    }

    if (entry.has(FLG_UNREADABLE)) {
        describer->body_str = "spellbooks";
        after_str[after_n++] = "of different realms from yours";
    }

    if (entry.has(FLG_REALM1)) {
        describer->body_str = "spellbooks";
        after_str[after_n++] = "of your first realm";
    }

    if (entry.has(FLG_REALM2)) {
        describer->body_str = "spellbooks";
        after_str[after_n++] = "of your second realm";
    }

    if (entry.has(FLG_FIRST)) {
        before_strings.emplace_back("first one of four");
        describer->body_str = "spellbooks";
    }

    if (entry.has(FLG_SECOND)) {
        before_strings.emplace_back("second one of four");
        describer->body_str = "spellbooks";
    }

    if (entry.has(FLG_THIRD)) {
        before_strings.emplace_back("third one of four");
        describer->body_str = "spellbooks";
    }

    if (entry.has(FLG_FOURTH)) {
        before_strings.emplace_back("fourth one of four");
        describer->body_str = "spellbooks";
    }

    if (entry.has(FLG_ITEMS)) {
        ;
    } /* Nothing to do */
    else if (entry.has(FLG_WEAPONS)) {
        describer->body_str = "weapons";
    } else if (entry.has(FLG_FAVORITE_WEAPONS)) {
        describer->body_str = "favorite weapons";
    } else if (entry.has(FLG_ARMORS)) {
        describer->body_str = "pieces of armor";
    } else if (entry.has(FLG_MISSILES)) {
        describer->body_str = "shots, arrows or crossbow bolts";
    } else if (entry.has(FLG_DEVICES)) {
        describer->body_str = "scrolls, wands, staffs or rods";
    } else if (entry.has(FLG_LIGHTS)) {
        describer->body_str = "light sources";
    } else if (entry.has(FLG_JUNKS)) {
        describer->body_str = "pieces of junk such as broken sticks";
    } else if (entry.has(FLG_CORPSES)) {
        describer->body_str = "corpses or skeletons";
    } else if (entry.has(FLG_SPELLBOOKS)) {
        describer->body_str = "spellbooks";
    } else if (entry.has(FLG_HAFTED)) {
        describer->body_str = "hafted weapons";
    } else if (entry.has(FLG_SHIELDS)) {
        describer->body_str = "shields";
    } else if (entry.has(FLG_BOWS)) {
        describer->body_str = "slings, bows or crossbows";
    } else if (entry.has(FLG_RINGS)) {
        describer->body_str = "rings";
    } else if (entry.has(FLG_AMULETS)) {
        describer->body_str = "amulets";
    } else if (entry.has(FLG_SUITS)) {
        describer->body_str = "pieces of body armor";
    } else if (entry.has(FLG_CLOAKS)) {
        describer->body_str = "cloaks";
    } else if (entry.has(FLG_HELMS)) {
        describer->body_str = "helms or crowns";
    } else if (entry.has(FLG_GLOVES)) {
        describer->body_str = "gloves";
    } else if (entry.has(FLG_BOOTS)) {
        describer->body_str = "boots";
    }

    if (*describer->str) {
        if (*describer->str == '^') {
            describer->str++;
            describer->top = true;
            whose_str[whose_n] = "names begin with \"";
            whose_arg_str[whose_n] = "";
            ++whose_n;
        } else {
            which_str[which_n++] = "have \"";
        }
    }

    if (describer->act.has(AutopickMethod::NOT_AUTOPICK)) {
        strcpy(buff, "Leave on floor ");
    } else if (describer->act.has(AutopickMethod::AUTODESTROY)) {
        strcpy(buff, "Destroy ");
    } else if (describer->act.has(AutopickMethod::QUERY_AUTOPICK)) {
        strcpy(buff, "Ask to pick up ");
    } else {
        strcpy(buff, "Pickup ");
    }

    if (describer->insc) {
        strncat(buff, format("and inscribe \"%s\"", describer->insc).data(), 80);

        if (str_find(describer->insc, "%all")) {
            strcat(buff, ", replacing %all with code string representing all abilities,");
        } else if (str_find(describer->insc, "%")) {
            strcat(buff, ", replacing % with code string representing extra random abilities,");
        }

        strcat(buff, " on ");
    }

    if (before_strings.empty()) {
        strcat(buff, "all ");
    } else {
        for (const auto &before_string : before_strings) {
            strcat(buff, before_string.data());
            strcat(buff, " ");
        }
    }

    strcat(buff, describer->body_str);
    for (int i = 0; i < after_n && after_str[i]; i++) {
        strcat(buff, " ");
        strcat(buff, after_str[i]);
    }

    for (int i = 0; i < whose_n && whose_str[i]; i++) {
        if (i == 0) {
            strcat(buff, " whose ");
        } else {
            strcat(buff, ", and ");
        }

        strcat(buff, whose_str[i]);
        strcat(buff, whose_arg_str[i]);
    }

    if (*describer->str && describer->top) {
        strcat(buff, describer->str);
        strcat(buff, "\"");
    }

    if (whose_n && which_n) {
        strcat(buff, ", and ");
    }

    for (int i = 0; i < which_n && which_str[i]; i++) {
        if (i == 0) {
            strcat(buff, " which ");
        } else {
            strcat(buff, ", and ");
        }

        strcat(buff, which_str[i]);
    }

    if (*describer->str && !describer->top) {
        strncat(buff, describer->str, 80);
        strcat(buff, "\" as part of their names");
    }

    strcat(buff, ".");

    if (describer->act.has(AutopickMethod::DISPLAY)) {
        if (describer->act.has(AutopickMethod::NOT_AUTOPICK)) {
            strcat(buff, "  Display these items when you press the N key in the full 'M'ap.");
        } else if (describer->act.has(AutopickMethod::AUTODESTROY)) {
            strcat(buff, "  Display these items when you press the K key in the full 'M'ap.");
        } else {
            strcat(buff, "  Display these items when you press the M key in the full 'M'ap.");
        }
    } else {
        strcat(buff, " Not displayed in the full map.");
    }
}
#endif

/*!
 * @brief Describe which kind of object is Auto-picked/destroyed
 */
void describe_autopick(char *buff, const autopick_type &entry)
{
    //! @note autopick_describer::str は non-nullable、autopick_describer::insc は nullable という制約がある
    autopick_describer describer{};
    describer.str = entry.name.data();
    describer.act = entry.action;
    describer.insc = entry.insc.empty() ? nullptr : entry.insc.data();
    describer.top = false;
    describer.body_str = _("アイテム", "items");
#ifdef JP
    describe_autpick_jp(buff, entry, &describer);
#else
    describe_autopick_en(buff, entry, &describer);
#endif
}
