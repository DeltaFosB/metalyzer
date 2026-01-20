#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

namespace metalyzer {

struct NFAState {
    int id;
    std::unordered_map<char, std::vector<NFAState*>> transitions;
    bool isAccepting = false;
    std::string tokenName = "";

    NFAState(int id) : id(id) {}
};

class NFAContext {
    std::vector<std::unique_ptr<NFAState>> registry;
    int nextId = 0;

public:
    NFAState* createState() {
        auto state = std::make_unique<NFAState>(nextId++);
        NFAState* ptr = state.get();
        registry.push_back(std::move(state));
        return ptr;
    }
};

struct NFA {
    NFAState* start = nullptr;
    NFAState* end = nullptr;
    std::vector<NFAState*> allStates; 
};

}

