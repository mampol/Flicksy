#pragma once

#include <string>
#include <vector>

struct MemoItem
{
    std::wstring id;
    std::wstring text;
    std::wstring created;
    std::wstring updated;

    std::vector<std::wstring> attachments;
};

class CMemoManager
{
public:
    CMemoManager() = default;

    static std::wstring MakeMemoId();
    static std::wstring GetCurrentDateTimeString();

    bool Load(const std::wstring& dataFolder);
    bool Save(const std::wstring& dataFolder) const;

    void Clear();

    size_t GetMemoCount() const;

    const MemoItem& GetMemo(size_t index) const;
    MemoItem& GetMemo(size_t index);

    size_t AddMemo(const MemoItem& memo);
    void SetMemo(size_t index, const MemoItem& memo);
    void DeleteMemo(size_t index);

private:
    std::vector<MemoItem> m_memos;
};
