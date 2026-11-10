#include "CMemoManager.h"

#include <windows.h>
#include <filesystem>
#include <fstream>
#include <toml.hpp>
#include "TextEncoding.h"

namespace
{
    constexpr wchar_t MEMO_FILE_NAME[] = L"fmemo.toml";

    std::filesystem::path MakeMemoFilePath(
        const std::wstring& dataFolder)
    {
        return std::filesystem::path(dataFolder)
            / MEMO_FILE_NAME;
    }
}

std::wstring CMemoManager::MakeMemoId()
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

    return buf;
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
    const std::wstring& dataFolder) const
{
    try
    {
        std::filesystem::path folder(dataFolder);

        if (!std::filesystem::exists(folder))
        {
            if (!std::filesystem::create_directories(folder))
                return false;
        }

        const std::filesystem::path path =
            MakeMemoFilePath(dataFolder);

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

