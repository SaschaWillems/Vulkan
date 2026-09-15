import display from '@ohos.display';

const DOMAIN = 0x0000;

@Entry
@Component
struct Index {
  @State message: string = 'Triangle'
  private context: object | undefined = undefined;

  aboutToAppear(): void {
    let config : display.Display = display.getDefaultDisplaySync();
    globalThis.context = getContext(this);
    globalThis.densityDPI = config.densityDPI;
  }

  build() {
    Stack() {
      XComponent({
        id: 'xcomponentId',
        type: XComponentType.SURFACE,
        libraryname: 'triangle'
      })
        .onLoad((xComponentContext) => {
          this.context = xComponentContext
        })
        .onDestroy(() => {
          console.info('onDestroy');
        })
        .id('xcomponent')
        .width('100%')
        .height('100%')
    }
    .width('100%')
    .height('100%')
  }
}
