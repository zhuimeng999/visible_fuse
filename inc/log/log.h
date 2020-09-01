//
// Created by lucius on 8/1/20.
//

#ifndef UNTITLED_LOG_H
#define UNTITLED_LOG_H

#include <boost/log/trivial.hpp>

#define CHECK(expr) \
            if (const auto res = !(expr)){ \
              BOOST_LOG_TRIVIAL(fatal) << "  "<< __FILE__ << " line " << __LINE__ << ", check " << #expr << " failed"; \
              exit(EXIT_FAILURE);\
            }

#define CHECK_VULKAN(fun) \
            while(const VkResult result = fun) { \
                BOOST_LOG_TRIVIAL(fatal) << "function " << #fun << " return " << result << ", not success"; \
                exit(EXIT_FAILURE); \
            }

#define CHECK_AND_LOG(expr, msg) \
            if (!(expr)){ \
              BOOST_LOG_TRIVIAL(fatal) << "  "<< __FILE__ << " line " << __LINE__ << ", check " << #expr << " failed, msg " << msg; \
              exit(EXIT_FAILURE);\
            }

#endif //UNTITLED_LOG_H
