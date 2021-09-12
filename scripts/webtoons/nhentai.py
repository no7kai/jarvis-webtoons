from typing import List
import bs4 as BeautifulSoup
import asyncio

from toonbase import ToonBase, AsyncToonMixin
import sys
from glob import glob


class NHentaiToon(AsyncToonMixin, ToonBase):
    @property
    def url(self) -> str:
        return f'https://nhentai.net/g/{self.episode}'

    async def resolve_name(self) -> str:
        soup = BeautifulSoup.BeautifulSoup(await self.get_page_content(), 'lxml')
        title = soup.find('meta', {'itemprop': 'name'})['content']
        return title.replace(' | ', ' ').replace('~', '')

    async def pages(self) -> List[str]:
        soup = BeautifulSoup.BeautifulSoup(await self.get_page_content(), 'lxml')
        thumbs_div = soup.find('div', 'thumbs')
        img_find = thumbs_div.find_all('img', {'class': 'lazyload'})

        def thumb_to_img(thumb_url: str) -> str:
            parts = thumb_url.split('/')
            sauce_code = parts[4]
            filename = parts[-1].replace('t', '')
            return f'https://cdn.nhentai.xxx/g/{sauce_code}/{filename}'

        imgs = [thumb_to_img(img['data-src']) for img in img_find]
        return imgs

    def save(self):
        pass

    @property
    def path(self):
        return f'/mnt/aiur/Users/snicolet/Scans/Toons/nhentai'

    @property
    def cbz_path(self):
        if not self.episode:
            return ''
        return f'{self.path}/[{self.episode:6}] {self.name}.cbz'

    async def pull(self, *args, **kwargs):
        if self.exists():
            return
        await super().pull(*args, **kwargs)

    def exists(self):
        return bool(glob(f'{self.path}/?{self.episode:6}]*.cbz'))



async def get_scan(sauce_code: int, name: str = None) -> bool:
    """Perform the download of sauce_code if not present,
    return True is a request has been performed, Flase otherwise
    """
    instance = NHentaiToon(
        name=name,
        episode=sauce_code,
        domain='https://nhentai.xxx'
    )
    if instance.exists():
        return False
    if not name:
        instance.name = await instance.resolve_name()
    await instance.pull(pool_size=5)
    return True


async def get_scan_list(sauce_list: List[int]) -> None:
    for sauce in sauce_list:
        if await get_scan(sauce):
            await asyncio.sleep(0.2)


if __name__ == "__main__":
    try:
        sauce_list = [int(sauce) for sauce in sys.argv[1:]]
        asyncio.run(get_scan_list(sauce_list))
    except (IndexError, ValueError):
        print(f'usage: ./{sys.argv[0]} [sauce_code]')
