#include <cstdio>
#include <cstring>
#include <archive.h>
#include <archive_entry.h>
#include <zstd.h>

struct ABC {
  int m_state {0};
  int m_expected {0};

  FILE* m_fpw;
  ZSTD_CCtx* m_ctx;

  const size_t m_szBufIn {ZSTD_CStreamInSize()};
  const size_t m_szBufOut {ZSTD_CStreamOutSize()};
  char* m_bufIn;
  char* m_bufOut;
  size_t m_posIn {0};

  ABC(const char* fname) : m_fpw{fopen(fname, "wb")},
    m_ctx {ZSTD_createCCtx()},
    m_bufIn {new char[m_szBufIn]},
    m_bufOut {new char[m_szBufOut]}
    {}

  ~ABC() {
    fclose(m_fpw);
    ZSTD_freeCCtx(m_ctx);
    delete [] m_bufIn;
    delete [] m_bufOut;
  }

  void resetZstdSession() {
    ZSTD_CCtx_reset(m_ctx, ZSTD_reset_session_and_parameters);
    if (m_state % 2 == 0) { // Content
      ZSTD_CCtx_setParameter(m_ctx, ZSTD_c_compressionLevel, 13);
      ZSTD_CCtx_setParameter(m_ctx, ZSTD_c_strategy, 6);
    } else { // Header
      ZSTD_CCtx_setParameter(m_ctx, ZSTD_c_compressionLevel, 22);
      ZSTD_CCtx_setParameter(m_ctx, ZSTD_c_strategy, 9);
    }
    ZSTD_CCtx_setParameter(m_ctx, ZSTD_c_nbWorkers, 2);
  }

  void changeState() {
    ++m_expected;
  }

  bool isStateChanged() {
    if (m_state < m_expected) {
      m_state = m_expected;
      return true;
    }
    return false;
  }

  void writeData(const char* buf, size_t sz) {
    if (m_posIn + sz >= m_szBufIn) {
      flushData();
    }
    printf("Ingested %lu bytes into input buffer (pos=%lu)\n", sz, m_posIn);
    memcpy(m_bufIn+m_posIn, buf, sz);
    m_posIn += sz;

  }

  void flushData() {
    if (m_posIn == 0) {
      resetZstdSession();
      return;
    }
    ZSTD_inBuffer objIn {m_bufIn, m_posIn, 0};
    size_t szRemain;
    do {
      ZSTD_outBuffer objOut {m_bufOut, m_szBufOut, 0};
      szRemain = ZSTD_compressStream2(m_ctx, &objOut ,&objIn, ZSTD_e_end);
      fwrite(m_bufOut, 1, objOut.pos, m_fpw);
      printf("Written %d bytes of out data, remaining=%lu\n", objOut.pos, szRemain);
    } while (szRemain != 0);
    printf("Flushed %lu bytes of input data\n", m_posIn);
    resetZstdSession();
    m_posIn = 0;
  }
};

int cbOpen(struct archive*, void *extra) {
  return ARCHIVE_OK;
}

int cbClose(struct archive*, void *extra) {
  printf("CLOSE %lx\n", extra);
  ABC* pabc {reinterpret_cast<ABC*>(extra)};
  pabc->flushData();
  return ARCHIVE_OK;
}

ssize_t cbWrite(struct archive*, void *extra, const void *buffer, size_t length) {
  printf("WRITE %lx SIZE %d\n", extra, length);
  ABC* pabc {reinterpret_cast<ABC*>(extra)};
  if (pabc->isStateChanged()) {
    pabc->flushData();
  }
  pabc->writeData(reinterpret_cast<const char*>(buffer), length);
  return length;
}

int main() {
  struct archive* pArchive = archive_write_new();

  archive_write_set_format_pax_restricted(pArchive);
  archive_write_set_bytes_per_block(pArchive, 0);
  archive_write_set_bytes_in_last_block(pArchive, 1);

  ABC abc {"out.tar.zst"};

  auto rtn = archive_write_open(pArchive, reinterpret_cast<void*>(&abc), cbOpen, cbWrite, cbClose);
  printf("open file: %d %s\n", rtn, archive_error_string(pArchive));

  const char* data {"abcdefghijklmnopqrstuvwxyz"};

  struct archive_entry* pEntry = archive_entry_new();
  archive_entry_set_pathname(pEntry, "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890.txt");
  archive_entry_set_filetype(pEntry, AE_IFREG);
  archive_entry_set_perm(pEntry, 0755);
  archive_entry_set_size(pEntry, 10);

  abc.changeState();
  rtn = archive_write_header(pArchive, pEntry);
  printf("write_header: %d %s\n", rtn, archive_error_string(pArchive));
  abc.changeState();
  rtn = archive_write_data(pArchive, data+0, 10);
  printf("write_data: %d %s\n", rtn, archive_error_string(pArchive));

  archive_write_finish_entry(pArchive);
  archive_entry_free(pEntry);

  pEntry = archive_entry_new();
  archive_entry_set_pathname(pEntry, "file2.txt");
  archive_entry_set_filetype(pEntry, AE_IFREG);
  archive_entry_set_perm(pEntry, 0755);
  archive_entry_set_size(pEntry, 15);

  abc.changeState();
  rtn = archive_write_header(pArchive, pEntry);
  printf("write_header: %d %s\n", rtn, archive_error_string(pArchive));
  abc.changeState();
  rtn = archive_write_data(pArchive, data+7, 15);
  printf("write_data: %d %s\n", rtn, archive_error_string(pArchive));

  archive_write_finish_entry(pArchive);
  archive_entry_free(pEntry);


  abc.changeState();
  archive_write_free(pArchive);

  return 0;
}
