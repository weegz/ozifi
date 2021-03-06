#include <contrib/opus/opus.h>

#include "compress.h"

namespace NVocal {

class TOpus: public TCodec {
public:
    TOpus();
    void Encode(TBuffer data, std::string& out);
    void Decode(TBuffer data, std::string& out);
private:
    OpusEncoder* Encoder;
    OpusDecoder* Decoder;
};

TOpus::TOpus() {
    Encoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, nullptr);
    Decoder = opus_decoder_create(48000, 1, nullptr);
}

void TOpus::Encode(TBuffer data, std::string& out) {
    out.resize(data.Size());
    int size = opus_encode(Encoder, (const opus_int16*)data.Data(), data.Size() / 2, (unsigned char*)&out[0], out.size());
    out.resize(size);
}

void TOpus::Decode(TBuffer data, std::string& out) {
    out.resize(1024);
    int size = opus_decode(Decoder, (const unsigned char*)data.Data(), data.Size(), (opus_int16*)&out[0], out.size() / 2, 0);
    out.resize(size * 2);
    assert(size < 1024 && "not all data decoded");
}

TCodec* CreateOpusCodec() {
    return new TOpus();
}

}
