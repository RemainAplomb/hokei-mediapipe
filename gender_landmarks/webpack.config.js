
const path  = require('path')

module.exports = {
  mode: 'development',
  entry: './gender_landmarks_demo.ts',
  module: {
      rules: [
          {
              test: /\.tsx?$/,
              use: 'babel-loader',
              exclude: /node_modules/,
          }
      ]
  },
  resolve: {
      extensions: ['.tsx', '.ts', '.js'],
  },
  output: {
    filename: 'gender_landmarks_demo.js',
    path: path.resolve(__dirname, 'dist')
  },
  devServer: {
    static: "./dist",
  },
}