import WinHelpers from '~/helpers/winHelpers'

const WIDTH = 350;
const HEIGHT = 615;

class WinManager {
  winHelpers = null;
  store = undefined;

  constructor() {
    this.winHelpers = new WinHelpers()
    this.store = {
      currentPopupId: undefined,
    }
  }

  /**
   * Either brings an existing notification window into focus, or creates a new notification window. New
   * notification windows are given a 'popup' type.
   *
   * @param {string} page - route path
   * @param {Function} cd - callback
   */
  async openWindow(page: string, cd?:(id: number) => void) {
    const popup = await this._getPopup();
    console.log('page:', page,'| popup', popup,'|',self);
    // Bring focus to chrome popup
    if (popup?.id) {
      await this.winHelpers.closeWindowById(popup.id);
      console.log('close window:',popup.id);
    }
      // create new notification popup
      let left = 0;
      let top = 0;
      let id;
      try {
        const lastFocused = await this.winHelpers.getLastFocusedWindow();
        // Position window in top right corner of lastFocused window.
        top = lastFocused.top;
        left = lastFocused.left + (lastFocused.width - WIDTH);

        if (lastFocused.id && lastFocused.type==='popup') id = lastFocused.id;
        if(id) await this.winHelpers.closeWindowById(id);
      } catch (_) {
        // The following properties are more than likely 0, due to being
        // opened from the background chrome process for the extension that
        // has no physical dimensions
        const windowDetails = await this.winHelpers.getCurrentWindow();
        top = Math.max(windowDetails?.height, 0);
        left = Math.max(windowDetails?.width - WIDTH, 0);
        if (windowDetails.id && windowDetails.type==='popup') id = windowDetails.id;
        if(id) await this.winHelpers.closeWindowById(id);
      }
      const popupWindow = await this.winHelpers.openWindow({
        url: chrome.runtime.getURL(`popup/index.html?page=${page}`),
        type: 'popup',
        width: WIDTH,
        height: HEIGHT,
        left,
        top,
      });

      // Callback
      if (typeof cd === 'function') {
        cd(popupWindow.id)
      }

      // Firefox currently ignores left/top for create, but it works for update
      if (popupWindow.left !== left && popupWindow.state !== 'fullscreen') {
        await this.winHelpers.updateWindowPosition(popupWindow.id, left, top);
      }
      // pass new created popup window id to appController setter
      // and store the id to private variable this.store.currentPopupId for future access
      this.store.currentPopupId = popupWindow.id;
  }

  /**
   * Checks all open windows, and returns the first one it finds that is a notification window (i.e. has the
   * type 'popup')
   *
   * @private
   */
  async _getPopup() {
    const windows = await this.winHelpers.getAllWindows();
    return this._getPopupIn(windows);
  }

  /**
   * Given an array of windows, returns the 'popup' that has been opened, or null if no such window exists.
   *
   * @private
   * @param {Array} windows - An array of objects containing data about the open extension windows.
   */
  _getPopupIn(windows) {
    const myself = this;
    return windows
      ? windows.find((win) => {
          // Returns notification popup
          return win && win.type === 'popup' && win.id === myself.store.currentPopupId;
        })
      : null;
  }
}

export default WinManager
