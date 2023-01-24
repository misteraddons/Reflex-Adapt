use std::collections::HashMap;

pub struct PageMap {
    pages: HashMap<usize, Vec<u8>>,
    page_size: usize,
}

impl PageMap {
    pub fn new(page_size: usize) -> PageMap {
        PageMap {
            pages: HashMap::new(),
            page_size: page_size,
        }
    }

    pub fn add_bytes(&mut self, offset: usize, data: &[u8]) {
        let mut page_addr = (offset / self.page_size) * self.page_size;
        let mut page_offset = offset % self.page_size;
        let mut data_offset: usize = 0;

        while data_offset != data.len() {
            let chunk_size =
                ((self.page_size - page_offset) as usize).min(data.len() - data_offset) as usize;
            let chunk = &data[data_offset..data_offset + chunk_size];
            let page_data = self
                .pages
                .entry(page_addr)
                .or_insert(vec![0; self.page_size]);

            page_data[page_offset..page_offset + chunk_size].copy_from_slice(chunk);

            page_addr += self.page_size;
            page_offset = 0;
            data_offset += chunk_size;
        }
    }

    pub fn get_page_addrs(&self) -> Vec<usize> {
        let mut addrs: Vec<usize> = self.pages.keys().cloned().collect();
        addrs.sort();
        addrs
    }

    pub fn get_page(&self, addr: usize) -> Option<&Vec<u8>> {
        self.pages.get(&addr)
    }
}
