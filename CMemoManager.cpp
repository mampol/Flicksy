#include "CMemoManager.h"

#include <windows.h>
#include <fstream>
#include <toml.hpp>
#include "TextEncoding.h"

namespace
{
    constexpr wchar_t MEMO_FILE_NAME[] = L"fmemo.toml";

    std::filesystem::path ResolveDataFolder(
        const std::wstring& dataFolder)
    {
        std::filesystem::path path(dataFolder);

        if (path.is_absolute())
        {
            return path;
        }

        wchar_t exePath[MAX_PATH]{};

        GetModuleFileName(
            nullptr,
            exePath,
            MAX_PATH);

        std::filesystem::path exeDir(exePath);
        exeDir = exeDir.parent_path();

        return exeDir / path;
    }

    std::filesystem::path MakeMemoFilePath(
        const std::wstring& dataFolder)
    {
        std::filesystem::path folder = ResolveDataFolder(dataFolder);
        return std::filesystem::path(folder) / MEMO_FILE_NAME;
    }
}

std::wstring CMemoManager::MakeMemoId() const
{
    SYSTEMTIME st{};
    GetLocalTime(&st);

    wchar_t buf[64]{};
    swprintf_s(
        buf,
        L"%04d%02d%02d_%02d%02d%02d_%03d",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond,
        st.wMilliseconds);

    std::wstring baseId = buf;
    std::wstring id = baseId;

    int cnt = 1;

    auto exists = [&](const std::wstring& target)
        {
            for (const auto& memo : m_memos)
            {
                if (memo.id == target)
                    return true;
            }

            return false;
        };

    while (exists(id))
    {
        id =
            baseId + L"_" +
            std::to_wstring(cnt);

        cnt++;
    }

    return id;
}

std::wstring CMemoManager::GetCurrentDateTimeString()
{
    SYSTEMTIME st{};
    GetLocalTime(&st);

    wchar_t buf[64]{};

    swprintf_s(
        buf,
        L"%04d-%02d-%02dT%02d:%02d:%02d",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond);

    return buf;
}

bool CMemoManager::Load(const std::wstring& dataFolder)
{
    std::vector<MemoItem> loadedMemos;

    const auto path = MakeMemoFilePath(dataFolder);

    if (!std::filesystem::exists(path))
    {
        m_memos.clear();
        return true;
    }

    try
    {
        toml::table tbl =
            toml::parse_file(path.string());

        if (auto* memos = tbl["memo"].as_array())
        {
            for (auto& node : *memos)
            {
                auto* item = node.as_table();
                if (!item) continue;

                MemoItem memo;

                memo.id = TextEncoding::Utf8ToUtf16(
                    (*item)["id"].value_or<std::string>(""));

                memo.text = TextEncoding::Utf8ToUtf16(
                    (*item)["text"].value_or<std::string>(""));

                memo.created = TextEncoding::Utf8ToUtf16(
                    (*item)["created"].value_or<std::string>(""));

                memo.updated = TextEncoding::Utf8ToUtf16(
                    (*item)["updated"].value_or<std::string>(""));

                if (auto* attachments =
                    (*item)["attachments"].as_array())
                {
                    for (auto& attachment : *attachments)
                    {
                        if (auto value =
                            attachment.value<std::string>())
                        {
                            memo.attachments.push_back(
                                TextEncoding::Utf8ToUtf16(*value));
                        }
                    }
                }

                loadedMemos.push_back(std::move(memo));
            }
        }

        m_memos = std::move(loadedMemos);

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool CMemoManager::Save(
    const std::wstring & dataFolder) const
{
    try
    {
        const auto path = MakeMemoFilePath(dataFolder);

        if (!std::filesystem::exists(path.parent_path()))
        {
            if (!std::filesystem::create_directories(path.parent_path()))
                return false;
        }

        toml::table tbl;
        toml::array memos;

        for (const auto& memo : m_memos)
        {
            toml::table item;

            item.insert(
                "id",
                TextEncoding::Utf16ToUtf8(memo.id));

            item.insert(
                "text",
                TextEncoding::Utf16ToUtf8(memo.text));

            item.insert(
                "created",
                TextEncoding::Utf16ToUtf8(memo.created));

            item.insert(
                "updated",
                TextEncoding::Utf16ToUtf8(memo.updated));

            toml::array attachments;

            for (const auto& attachment :
                memo.attachments)
            {
                attachments.push_back(
                    TextEncoding::Utf16ToUtf8(attachment));
            }

            item.insert(
                "attachments",
                std::move(attachments));

            memos.push_back(
                std::move(item));
        }

        tbl.insert(
            "memo",
            std::move(memos));

        std::ofstream ofs(
            path,
            std::ios::binary |
            std::ios::trunc);

        if (!ofs)
            return false;

        ofs << tbl;

        return ofs.good();
    }
    catch (...)
    {
        return false;
    }
}

void CMemoManager::Clear()
{
    m_memos.clear();
}

size_t CMemoManager::GetMemoCount() const
{
    return m_memos.size();
}

const MemoItem&
CMemoManager::GetMemo(size_t index) const
{
    return m_memos.at(index);
}

MemoItem&
CMemoManager::GetMemo(size_t index)
{
    return m_memos.at(index);
}

size_t CMemoManager::AddMemo(
    const MemoItem& memo)
{
    m_memos.push_back(memo);

    return m_memos.size() - 1;
}

void CMemoManager::SetMemo(
    size_t index,
    const MemoItem& memo)
{
    m_memos.at(index) = memo;
}

void CMemoManager::DeleteMemo(
    size_t index)
{
    if (index >= m_memos.size())
        return;

    m_memos.erase(
        m_memos.begin() + index);
}

