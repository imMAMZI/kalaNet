#include "models/ad.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <QJsonValue>
#include <QString>

namespace {

std::string currentTimestamp() {
    using namespace std::chrono;

    const auto now = system_clock::now();
    const std::time_t rawTime = system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&rawTime), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

QJsonObject categoryToJson(const common::Category& category) {
    QJsonObject obj;
    obj.insert("id", category.id());
    obj.insert("name", QString::fromStdString(category.name()));
    return obj;
}

common::Category categoryFromJson(const QJsonObject& json) {
    return common::Category(
        json.value("id").toInt(),
        json.value("name").toString().toStdString()
    );
}

} // namespace

namespace common {

std::string toString(AdStatus status) {
    switch (status) {
        case AdStatus::Pending:  return "PENDING";
        case AdStatus::Approved: return "APPROVED";
        case AdStatus::Rejected: return "REJECTED";
        case AdStatus::Sold:     return "SOLD";
        case AdStatus::Archived: return "ARCHIVED";
    }
    throw std::logic_error("Unhandled AdStatus value");
}

AdStatus adStatusFromString(const std::string& value) {
    if (value == "PENDING")  return AdStatus::Pending;
    if (value == "APPROVED") return AdStatus::Approved;
    if (value == "REJECTED") return AdStatus::Rejected;
    if (value == "SOLD")     return AdStatus::Sold;
    if (value == "ARCHIVED") return AdStatus::Archived;
    throw std::invalid_argument("Unknown AdStatus string: " + value);
}

QJsonObject AdStatusEntry::toJson() const {
    QJsonObject obj;
    obj.insert("status", QString::fromStdString(toString(status)));
    obj.insert("changedAt", QString::fromStdString(changedAt));
    if (note.has_value()) {
        obj.insert("note", QString::fromStdString(*note));
    }
    return obj;
}

AdStatusEntry AdStatusEntry::fromJson(const QJsonObject& json) {
    AdStatusEntry entry;
    entry.status = adStatusFromString(json.value("status").toString().toStdString());
    entry.changedAt = json.value("changedAt").toString().toStdString();
    if (json.contains("note")) {
        entry.note = json.value("note").toString().toStdString();
    }
    return entry;
}

QJsonObject AdSummaryDTO::toJson() const {
    QJsonObject obj;
    obj.insert("id", id);
    obj.insert("title", QString::fromStdString(title));
    obj.insert("price", price);
    obj.insert("status", QString::fromStdString(toString(status)));
    obj.insert("category", categoryToJson(category));
    obj.insert("seller", QString::fromStdString(sellerUsername));
    obj.insert("thumbnailPath", QString::fromStdString(thumbnailPath));
    return obj;
}

AdSummaryDTO AdSummaryDTO::fromJson(const QJsonObject& json) {
    AdSummaryDTO dto;
    dto.id = json.value("id").toInt(-1);
    dto.title = json.value("title").toString().toStdString();
    dto.price = json.value("price").toDouble();
    dto.status = adStatusFromString(json.value("status").toString().toStdString());
    dto.category = categoryFromJson(json.value("category").toObject());
    dto.sellerUsername = json.value("seller").toString().toStdString();
    dto.thumbnailPath = json.value("thumbnailPath").toString().toStdString();
    return dto;
}

AdSummaryDTO AdSummaryDTO::fromModel(const Ad& ad) {
    AdSummaryDTO dto;
    dto.id = ad.id();
    dto.title = ad.title();
    dto.price = ad.price();
    dto.status = ad.status();
    dto.category = ad.category();
    dto.sellerUsername = ad.seller();
    dto.thumbnailPath = ad.imagePath();
    return dto;
}

QJsonObject AdDetailDTO::toJson() const {
    QJsonObject obj;
    obj.insert("id", id);
    obj.insert("title", QString::fromStdString(title));
    obj.insert("description", QString::fromStdString(description));
    obj.insert("price", price);
    obj.insert("status", QString::fromStdString(toString(status)));
    obj.insert("seller", QString::fromStdString(sellerUsername));
    obj.insert("imagePath", QString::fromStdString(imagePath));
    obj.insert("createdAt", QString::fromStdString(createdAt));
    obj.insert("updatedAt", QString::fromStdString(updatedAt));
    obj.insert("category", categoryToJson(category));

    if (rejectionReason.has_value()) {
        obj.insert("rejectionReason", QString::fromStdString(*rejectionReason));
    }

    QJsonArray tagsJson;
    for (const auto& tag : tags) {
        tagsJson.push_back(QString::fromStdString(tag));
    }
    obj.insert("tags", tagsJson);

    QJsonArray historyJson;
    for (const auto& entry : history) {
        historyJson.push_back(entry.toJson());
    }
    obj.insert("history", historyJson);

    return obj;
}

AdDetailDTO AdDetailDTO::fromJson(const QJsonObject& json) {
    AdDetailDTO dto;
    dto.id = json.value("id").toInt(-1);
    dto.title = json.value("title").toString().toStdString();
    dto.description = json.value("description").toString().toStdString();
    dto.price = json.value("price").toDouble();
    dto.status = adStatusFromString(json.value("status").toString().toStdString());
    dto.sellerUsername = json.value("seller").toString().toStdString();
    dto.imagePath = json.value("imagePath").toString().toStdString();
    dto.createdAt = json.value("createdAt").toString().toStdString();
    dto.updatedAt = json.value("updatedAt").toString().toStdString();
    dto.category = categoryFromJson(json.value("category").toObject());

    if (json.contains("rejectionReason")) {
        dto.rejectionReason = json.value("rejectionReason").toString().toStdString();
    }

    const auto tagsJson = json.value("tags").toArray();
    dto.tags.reserve(tagsJson.size());
    for (const auto& value : tagsJson) {
        dto.tags.push_back(value.toString().toStdString());
    }

    const auto historyJson = json.value("history").toArray();
    dto.history.reserve(historyJson.size());
    for (const auto& value : historyJson) {
        dto.history.push_back(AdStatusEntry::fromJson(value.toObject()));
    }

    return dto;
}

AdDetailDTO AdDetailDTO::fromModel(const Ad& ad) {
    AdDetailDTO dto;
    dto.id = ad.id();
    dto.title = ad.title();
    dto.description = ad.description();
    dto.price = ad.price();
    dto.status = ad.status();
    dto.sellerUsername = ad.seller();
    dto.imagePath = ad.imagePath();
    dto.createdAt = ad.createdAt();
    dto.updatedAt = ad.updatedAt();
    dto.category = ad.category();
    dto.rejectionReason = ad.rejectionReason();
    dto.tags = ad.tags();
    dto.history = ad.statusHistory();
    return dto;
}

Ad::Ad()
    : category_(),
      seller_(),
      status_(AdStatus::Pending)
{
    touchTimestamps(true);
    appendHistory(status_, std::nullopt);
}

Ad::Ad(int id,
       std::string title,
       std::string description,
       double price,
       Category category,
       std::string sellerUsername,
       std::string imagePath,
       std::vector<std::string> tags,
       AdStatus status,
       std::string createdAt,
       std::string updatedAt,
       std::optional<std::string> rejectionReason,
       std::vector<AdStatusEntry> history)
    : id_(id),
      title_(std::move(title)),
      description_(std::move(description)),
      price_(price),
      category_(std::move(category)),
      seller_(std::move(sellerUsername)),
      imagePath_(std::move(imagePath)),
      tags_(std::move(tags)),
      status_(status),
      rejectionReason_(std::move(rejectionReason)),
      createdAt_(std::move(createdAt)),
      updatedAt_(std::move(updatedAt)),
      history_(std::move(history))
{
    if (createdAt_.empty()) {
        touchTimestamps(true);
    } else if (updatedAt_.empty()) {
        updatedAt_ = createdAt_;
    }
    if (history_.empty()) {
        appendHistory(status_, rejectionReason_);
    }
}

int Ad::id() const {
    return id_;
}

void Ad::setId(int id) {
    id_ = id;
}

const std::string& Ad::title() const {
    return title_;
}

void Ad::setTitle(const std::string& title) {
    title_ = title;
    touchTimestamps();
}

const std::string& Ad::description() const {
    return description_;
}

void Ad::setDescription(const std::string& description) {
    description_ = description;
    touchTimestamps();
}

double Ad::price() const {
    return price_;
}

void Ad::setPrice(double price) {
    price_ = price;
    touchTimestamps();
}

const Category& Ad::category() const {
    return category_;
}

void Ad::setCategory(const Category& category) {
    category_ = category;
    touchTimestamps();
}

const std::string& Ad::seller() const {
    return seller_;
}

const std::string& Ad::imagePath() const {
    return imagePath_;
}

void Ad::setImagePath(const std::string& imagePath) {
    imagePath_ = imagePath;
    touchTimestamps();
}

const std::vector<std::string>& Ad::tags() const {
    return tags_;
}

void Ad::setTags(const std::vector<std::string>& tags) {
    tags_ = tags;
    touchTimestamps();
}

AdStatus Ad::status() const {
    return status_;
}

const std::optional<std::string>& Ad::rejectionReason() const {
    return rejectionReason_;
}

const std::string& Ad::createdAt() const {
    return createdAt_;
}

const std::string& Ad::updatedAt() const {
    return updatedAt_;
}

const std::vector<AdStatusEntry>& Ad::statusHistory() const {
    return history_;
}

void Ad::approve() {
    rejectionReason_.reset();
    setStatus(AdStatus::Approved);
}

void Ad::reject(const std::string& reason) {
    rejectionReason_ = reason;
    setStatus(AdStatus::Rejected, reason);
}

void Ad::markSold() {
    rejectionReason_.reset();
    setStatus(AdStatus::Sold);
}

void Ad::archive() {
    setStatus(AdStatus::Archived);
}

void Ad::resetToPending() {
    rejectionReason_.reset();
    setStatus(AdStatus::Pending);
}

AdSummaryDTO Ad::toSummaryDTO() const {
    return AdSummaryDTO::fromModel(*this);
}

AdDetailDTO Ad::toDetailDTO() const {
    return AdDetailDTO::fromModel(*this);
}

void Ad::setStatus(AdStatus status, std::optional<std::string> note) {
    if (status_ == status && note == rejectionReason_) {
        return; // no-op
    }
    status_ = status;
    touchTimestamps();
    appendHistory(status, note);
}

void Ad::touchTimestamps(bool touchCreated) {
    const auto now = currentTimestamp();
    if (touchCreated && createdAt_.empty()) {
        createdAt_ = now;
    }
    updatedAt_ = now;
}

void Ad::appendHistory(AdStatus status, const std::optional<std::string>& note) {
    history_.push_back(AdStatusEntry{
        status,
        currentTimestamp(),
        note
    });
}

} // namespace common
