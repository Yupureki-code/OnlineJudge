#pragma once

#include "control_solution.hpp"

namespace ns_control
{

    class ControlUser : public ControlSolution
    {
    public:
        bool UploadAvatar(const User& current_user, const std::string& file_content, const std::string& content_type, Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
                return false;

            static const size_t kMaxAvatarSize = 2 * 1024 * 1024;
            static const std::vector<std::string> kAllowedTypes = {"image/jpeg", "image/png", "image/gif", "image/webp"};

            bool type_allowed = false;
            for (const auto& t : kAllowedTypes)
            {
                if (content_type == t)
                {
                    type_allowed = true;
                    break;
                }
            }
            if (!type_allowed)
            {
                *err_code = "INVALID_TYPE";
                return false;
            }

            if (file_content.size() > kMaxAvatarSize)
            {
                *err_code = "FILE_TOO_LARGE";
                return false;
            }

            std::string ext;
            if (content_type == "image/jpeg") ext = "jpg";
            else if (content_type == "image/png") ext = "png";
            else if (content_type == "image/gif") ext = "gif";
            else ext = "webp";

            // 用户头像按 uid 命名，每个用户只保存一份
            std::string filename = std::to_string(current_user.uid) + "." + ext;
            std::string relative_path = std::string("/pictures/avatars/") + filename;
            std::string absolute_dir = std::string(HTML_PATH) + "pictures/avatars/";

            struct stat st;
            if (stat(absolute_dir.c_str(), &st) != 0)
            {
                mkdir(absolute_dir.c_str(), 0755);
            }

            // 删除旧头像文件（防止换扩展名后旧文件残留）
            if (!current_user.avatar_path.empty()
                && current_user.avatar_path.find("../") == std::string::npos
                && current_user.avatar_path.find("/pictures/avatars/") == 0
                && current_user.avatar_path != relative_path)
            {
                std::string old_path = std::string(HTML_PATH) + "pictures/avatars/" + std::string(current_user.avatar_path.substr(current_user.avatar_path.rfind('/') + 1));
                unlink(old_path.c_str());
            }

            std::string absolute_path = absolute_dir + filename;
            if (!ns_util::FileUtil::WriteFile(absolute_path, file_content))
            {
                *err_code = "SAVE_FAILED";
                return false;
            }

            if (!_model.UpdateUserAvatar(current_user.uid, relative_path))
            {
                unlink(absolute_path.c_str());
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["avatar_url"] = relative_path;
            _model.SetCachedStringByAnyKey("avatar:user:" + std::to_string(current_user.uid), relative_path, 3600);
            return true;
        }

        bool DeleteAvatar(const User& current_user, Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
                return false;

            // 删除磁盘上的旧头像文件（仅限 avatars 目录下的文件，防止路径穿越）
            if (!current_user.avatar_path.empty())
            {
                std::string rel = current_user.avatar_path;
                if (rel.find("../") == std::string::npos && rel.find("/pictures/avatars/") == 0)
                {
                    std::string absolute_path = std::string(HTML_PATH) + "pictures/avatars/" + std::string(rel.substr(rel.rfind('/') + 1));
                    unlink(absolute_path.c_str());
                }
            }

            if (!_model.UpdateUserAvatar(current_user.uid, ""))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            _model.SetCachedStringByAnyKey("avatar:user:" + std::to_string(current_user.uid), "", 3600);
            return true;
        }

        bool GetUserSubmits(const std::string& question_id, const User& current_user, Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
                return false;

            Json::Value submits(Json::arrayValue);
            if (!_model.GetUserSubmitsByQuestion(current_user.uid, question_id, &submits))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["submits"] = submits;
            return true;
        }

        bool GetUserStats(const User& current_user, Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
                return false;

            Json::Value stats;
            if (!_model.GetUserStats(current_user.uid, &stats))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["stats"] = stats;
            return true;
        }
    };

}
