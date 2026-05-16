#pragma once

#include "util/abstract-vector-wrapper.h"
#include <string_view>
#include <vector>

struct CommandMenuDatum {
public:
    CommandMenuDatum(std::string_view name, int level, int key, int com_id)
        : name(name)
        , level(level)
        , key(key)
        , com_id(com_id)
    {
    }

    std::string_view name = "";
    int level = 0;
    int key = 0;
    int com_id = 0;
};

class CommandMenuData : public util::AbstractVectorWrapper<CommandMenuDatum> {
public:
    CommandMenuData(CommandMenuData &&) = delete;
    CommandMenuData(const CommandMenuData &) = delete;
    CommandMenuData &operator=(const CommandMenuData &) = delete;
    CommandMenuData &operator=(CommandMenuData &&) = delete;

    static CommandMenuData &get_instance();

    void initialize();
    const CommandMenuDatum &get_datum(int num) const;
    int get_com_id(char key) const;

private:
    CommandMenuData() = default;

    static CommandMenuData instance;

    std::vector<CommandMenuDatum> menu_data;

    std::vector<CommandMenuDatum> &get_inner_container() override
    {
        return this->menu_data;
    }
};
