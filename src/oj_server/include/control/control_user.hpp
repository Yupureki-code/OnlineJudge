#pragma once

#include "control_base.hpp"

namespace ns_control
{

    class ControlUser : public ControlBase
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
            static const char* kExts[] = {".jpg", ".png", ".gif", ".webp"};
            for (const char* old_ext : kExts)
            {
                std::string old_path = absolute_dir + std::to_string(current_user.uid) + old_ext;
                if (old_path != absolute_dir + filename)
                {
                    unlink(old_path.c_str());
                }
            }

            std::string absolute_path = absolute_dir + filename;
            if (!ns_util::FileUtil::WriteFile(absolute_path, file_content))
            {
                *err_code = "SAVE_FAILED";
                return false;
            }

            (*result)["success"] = true;
            (*result)["avatar_url"] = relative_path;
            _model.DeleteCachedStringByAnyKey("avatar:user:" + std::to_string(current_user.uid));
            return true;
        }

        bool DeleteAvatar(const User& current_user, Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
                return false;

            // 删除磁盘上的所有旧头像文件
            std::string absolute_dir = std::string(HTML_PATH) + "pictures/avatars/";
            static const char* kExts[] = {".jpg", ".png", ".gif", ".webp"};
            for (const char* ext : kExts)
            {
                unlink((absolute_dir + std::to_string(current_user.uid) + ext).c_str());
            }

            (*result)["success"] = true;
            _model.DeleteCachedStringByAnyKey("avatar:user:" + std::to_string(current_user.uid));
            return true;
        }

        bool GetUserSubmits(const std::string& question_id, const User& current_user, Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
                return false;

            Json::Value submits(Json::arrayValue);
            if (!_model.User().GetUserSubmitsByQuestion(current_user.uid, question_id, &submits))
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
            if (!_model.User().GetUserStats(current_user.uid, &stats))
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
