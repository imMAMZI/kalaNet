#ifndef AD_H
#define AD_H

#pragma once

#include <optional>
#include <string>
#include <vector>

#include <QJsonArray>
#include <QJsonObject>

#include "models/category.h"

namespace common {

enum class AdStatus {
    Pending,
    Approved,
    Rejected,
    Sold,
    Archived
};

std::string toString(AdStatus status);
AdStatus adStatusFromString(const std::string& value);

struct AdStatusEntry {
    AdStatus status;
    std::string changedAt;
    std::optional<std::string> note;

    QJsonObject toJson() const;
    static AdStatusEntry fromJson(const QJsonObject& json);
};

struct AdSummaryDTO {
    int id = -1;
    std::string title;
    double price = 0.0;
    AdStatus status = AdStatus::Pending;
    Category category;
    std::string sellerUsername;
    std::string thumbnailPath;

    QJsonObject toJson() const;
    static AdSummaryDTO fromJson(const QJsonObject& json);
    static AdSummaryDTO fromModel(const class Ad& ad);
};

struct AdDetailDTO {
    int id = -1;
    std::string title;
    std::string description;
    double price = 0.0;
    Category category;
    std::string sellerUsername;
    std::string imagePath;
    AdStatus status = AdStatus::Pending;
    std::string createdAt;
    std::string updatedAt;
    std::optional<std::string> rejectionReason;
    std::vector<std::string> tags;
    std::vector<AdStatusEntry> history;

    QJsonObject toJson() const;
    static AdDetailDTO fromJson(const QJsonObject& json);
    static AdDetailDTO fromModel(const class Ad& ad);
};

class Ad {
public:
    Ad();

    Ad(int id,
       std::string title,
       std::string description,
       double price,
       Category category,
       std::string sellerUsername,
       std::string imagePath,
       std::vector<std::string> tags = {},
       AdStatus status = AdStatus::Pending,
       std::string createdAt = {},
       std::string updatedAt = {},
       std::optional<std::string> rejectionReason = std::nullopt,
       std::vector<AdStatusEntry> history = {});

    int id() const;
    void setId(int id);

    const std::string& title() const;
    void setTitle(const std::string& title);

    const std::string& description() const;
    void setDescription(const std::string& description);

    double price() const;
    void setPrice(double price);

    const Category& category() const;
    void setCategory(const Category& category);

    const std::string& seller() const;

    const std::string& imagePath() const;
    void setImagePath(const std::string& imagePath);

    const std::vector<std::string>& tags() const;
    void setTags(const std::vector<std::string>& tags);

    AdStatus status() const;
    const std::optional<std::string>& rejectionReason() const;

    const std::string& createdAt() const;
    const std::string& updatedAt() const;

    const std::vector<AdStatusEntry>& statusHistory() const;

    void approve();
    void reject(const std::string& reason);
    void markSold();
    void archive();
    void resetToPending();

    AdSummaryDTO toSummaryDTO() const;
    AdDetailDTO toDetailDTO() const;

private:
    void setStatus(AdStatus status, std::optional<std::string> note = std::nullopt);
    void touchTimestamps(bool touchCreated = false);
    void appendHistory(AdStatus status, const std::optional<std::string>& note);

    int id_ = -1;
    std::string title_;
    std::string description_;
    double price_ = 0.0;
    Category category_;
    std::string seller_;
    std::string imagePath_;
    std::vector<std::string> tags_;
    AdStatus status_ = AdStatus::Pending;
    std::optional<std::string> rejectionReason_;
    std::string createdAt_;
    std::string updatedAt_;
    std::vector<AdStatusEntry> history_;
};

} // namespace common

#endif // AD_H
