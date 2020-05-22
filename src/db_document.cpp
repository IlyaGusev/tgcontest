#include "db_document.h"

#include "util.h"

TDbDocument TDbDocument::FromProto(const tg::TDocumentProto& proto) {
    TDbDocument document;
    document.FileName = proto.filename();
    document.PubTime = proto.pubtime();
    document.Ttl = proto.ttl();
    document.Title = proto.title();
    document.Language = proto.language();
    document.Category = proto.category();

    for (const auto& embedding : proto.embeddings()) {
        const auto& valueProto = embedding.value();
        TEmbedding value(valueProto.cbegin(), valueProto.cend());

        const auto [_, success] = document.Embeddings.try_emplace(embedding.key(), std::move(value));
        ENSURE(success, "Unexpected key duplicate");
    }

    return document;
}

bool TDbDocument::FromProtoString(const std::string& value, TDbDocument* document) {
    tg::TDocumentProto proto;
    if (proto.ParseFromString(value)) {
        *document = FromProto(proto);
        return true;
    }
    return false;
}

tg::TDocumentProto TDbDocument::ToProto() const {
    tg::TDocumentProto proto;
    proto.set_filename(FileName);
    proto.set_pubtime(PubTime);
    proto.set_ttl(Ttl);
    proto.set_title(Title);
    proto.set_language(Language);
    proto.set_category(Category);

    for (const auto& [key, val] : Embeddings) {
        auto* embeddingProto = proto.add_embeddings();
        embeddingProto->set_key(key);
        *embeddingProto->mutable_value() = {val.cbegin(), val.cend()};
    }
    return proto;
}

bool TDbDocument::ToProtoString(std::string* protoString) const {
    return ToProto().SerializeToString(protoString);
}
