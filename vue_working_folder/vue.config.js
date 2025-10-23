const { defineConfig } = require('@vue/cli-service')
module.exports = defineConfig({
  transpileDependencies: true,
  productionSourceMap: false, // Disable source maps (~100-300 KB savings)
  css: { sourceMap: false },  // Disable CSS source maps
  configureWebpack: {
    optimization: {
      minimize: true
    }
  }
})
