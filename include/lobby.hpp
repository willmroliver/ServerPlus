#ifndef INCLUDE_LOBBY_H
#define INCLUDE_LOBBY_H

#include <unordered_map>
#include <string>
#include <functional>
#include <random>
#include "utility/rand.hpp"

namespace serv {

class LobbyInterface {
    public:
        virtual std::string add_user() = 0;
        virtual void clear_user(const std::string& uid) = 0;
        virtual bool mutate(std::string& mutation, std::string& unitary) = 0;
        virtual bool mutate(std::string& mutation) = 0;
};

template <typename Shared, typename Unitary>
class Lobby : public LobbyInterface {
    private:
        Shared lobby_state;
        std::unordered_map<std::string, std::unique_ptr<Unitary>> user_states;
        std::unordered_map<std::string, std::function<bool(Shared&, Unitary*)>> mutations;

    public:
        Lobby() = default;
        Lobby(const Shared& initial): lobby_state { initial } {}
        Lobby(const Shared&& initial): lobby_state { initial } {}
        ~Lobby() = default;

        Lobby& operator=(const Lobby& lobby) = default;
        Lobby& operator=(Lobby&& lobby) = default;

        std::string add_user() {
            std::string uid(16)

            do {
                uid = util::rand_alphanumeric_inplace(uid);
            } 
            while (user_states.find(uid) != user_states.end());

            user_states[uid] = std::make_unique<Unitary>(std::forward(args));

            return uid;
        }

        void clear_user(const std::string& uid) {
            user_states.erase(uid);
        }
        
        void set_mutation(std::string& mutation, std::function<bool(Shared&, Unitary*)> cb) {
            mutations[mutation] = cb;
        }

        bool mutate(std::string& mutation, std::string& unitary) {
            if (mutations.find(mutation) == mutations.end() || user_states.find(unitary) == user_states.end()) {
                return false;
            }

            return mutations[mutation](lobby_state, &(*user_states[unitary]));
        }

        bool mutate(std::string& mutation) {
            if (mutations.find(mutation) == mutations.end()) {
                return false;
            }

            return mutations[mutation](lobby_state, nullptr);
        }

        bool mutate(std::function<bool(Shared&, Unitary*)> cb, std::string unitary) {
            if (user_states.find(mutation) == user_states.end()) {
                return false;
            }

            return cb(lobby_state, user_states[unitary]);
        }

        bool mutate(std::function<bool(Shared&)> cb) {
            return cb(lobby_state);
        }
};

}

#endif