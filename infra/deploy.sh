#!/usr/bin/env bash
# Deploys Tunea tu Combi to the AWS account the AWS CLI is signed in to.
#
#   infra/deploy.sh [-y] backend    infrastructure and Lambdas
#   infra/deploy.sh web             the web build (build it first: `just web-release`)
#   infra/deploy.sh outputs         where everything ended up
#   infra/deploy.sh destroy         delete everything, players' data included
#
# The `just aws-*` recipes are the way in: they build the game and chain these.
#
# STACK (default tunea-tu-combi) names the stack. The account and the region
# are the ones of your AWS CLI profile: AWS_PROFILE and AWS_REGION change them.
set -euo pipefail
cd "$(dirname "$0")/.."

die() { echo "error: $*" >&2; exit 1; }

ASSUME_YES=0
COMMAND=
for argument in "$@"; do
    case "$argument" in
        -y|--yes) ASSUME_YES=1 ;;
        backend|web|outputs|destroy) COMMAND="$argument" ;;
        *) die "unknown argument '$argument' (see the top of this file)" ;;
    esac
done
[ -n "$COMMAND" ] || die "which command? backend, web, outputs or destroy (see the top of this file)"

STACK="${STACK:-tunea-tu-combi}"
[[ "$STACK" =~ ^[a-z][a-z0-9-]{0,40}$ ]] \
    || die "STACK must be lowercase letters, digits and dashes (it names an S3 bucket)"

for tool in aws node npm; do
    command -v "$tool" >/dev/null || die "'$tool' is not installed"
done

REGION="${AWS_REGION:-${AWS_DEFAULT_REGION:-$(aws configure get region 2>/dev/null || true)}}"
[ -n "$REGION" ] || die "no region: run 'aws configure' or set AWS_REGION (for example us-east-1)"
export AWS_DEFAULT_REGION="$REGION"
ACCOUNT="$(aws sts get-caller-identity --query Account --output text)" \
    || die "the AWS CLI is not signed in: run 'aws configure' or 'aws sso login'"
ARTIFACTS="$STACK-artifacts-$ACCOUNT-$REGION"

output() {
    aws cloudformation describe-stacks --stack-name "$STACK" \
        --query "Stacks[0].Outputs[?OutputKey=='$1'].OutputValue" --output text
}

confirm() {
    echo "Stack '$STACK' in account $ACCOUNT, region $REGION."
    [ "$ASSUME_YES" = 1 ] && return
    read -r -p "Create or update it? [y/N] " answer
    [[ "$answer" =~ ^[yY]$ ]] || die "cancelled"
}

# api/ with only what runs, and its production dependencies exactly as locked
stage_api() {
    rm -rf build/api build/packaged.yaml
    mkdir -p build/api
    cp -R api/functions build/api/functions
    cp api/*.mjs api/package.json api/package-lock.json build/api/
    (cd build/api && npm ci --omit=dev --ignore-scripts --no-audit --no-fund --loglevel=error)
}

# the template and the Lambdas' code are uploaded from here before deploying
ensure_artifact_bucket() {
    aws s3api head-bucket --bucket "$ARTIFACTS" 2>/dev/null && return
    echo "Creating the bucket for the deployment files: $ARTIFACTS"
    if [ "$REGION" = us-east-1 ]; then
        aws s3api create-bucket --bucket "$ARTIFACTS" >/dev/null
    else
        aws s3api create-bucket --bucket "$ARTIFACTS" \
            --create-bucket-configuration "LocationConstraint=$REGION" >/dev/null
    fi
    aws s3api put-public-access-block --bucket "$ARTIFACTS" \
        --public-access-block-configuration \
        BlockPublicAcls=true,IgnorePublicAcls=true,BlockPublicPolicy=true,RestrictPublicBuckets=true
}

deploy_backend() {
    confirm
    stage_api
    ensure_artifact_bucket
    aws cloudformation package --template-file infra/template.yaml \
        --s3-bucket "$ARTIFACTS" --output-template-file build/packaged.yaml >/dev/null
    echo "Deploying (the first time CloudFront takes about ten minutes)..."
    aws cloudformation deploy --template-file build/packaged.yaml --stack-name "$STACK" \
        --capabilities CAPABILITY_IAM CAPABILITY_AUTO_EXPAND \
        --no-fail-on-empty-changeset --tags project=tunea-tu-combi
}

upload() {
    aws s3 cp "$1" "s3://$SITE_BUCKET/$2" --content-type "$3" --cache-control no-cache >/dev/null
    echo "  $2"
}

deploy_web() {
    local web=build/web-release
    [ -f "$web/ray.html" ] || die "there is no web build in $web: run 'just web-release' first"
    SITE_BUCKET="$(output SiteBucketName)"
    local distribution
    distribution="$(output DistributionId)"
    [ -n "$SITE_BUCKET" ] && [ "$SITE_BUCKET" != None ] \
        || die "stack '$STACK' is not deployed: run 'just aws-deploy-api' first"
    echo "Uploading the songs to $SITE_BUCKET (only what changed):"
    aws s3 sync assets/audio "s3://$SITE_BUCKET/assets/audio" \
        --exclude '*' --include 'clean/*.ogg' --include 'custom/*.ogg' \
        --size-only --cache-control 'public, max-age=86400' --only-show-errors
    echo "Uploading the game to $SITE_BUCKET:"
    upload "$web/ray.html" index.html text/html
    for file in "$web"/ray.*; do
        case "$file" in
            *.html) ;;
            *.js)   upload "$file" "$(basename "$file")" text/javascript ;;
            *.wasm) upload "$file" "$(basename "$file")" application/wasm ;;
            *.data) upload "$file" "$(basename "$file")" application/octet-stream ;;
        esac
    done
    upload "$web/thumbnail.jpg" thumbnail.jpg image/jpeg
    upload "$web/loading-cargando.png" loading-cargando.png image/png
    aws cloudfront create-invalidation --distribution-id "$distribution" --paths '/*' >/dev/null
}

present() { [ -n "$1" ] && [ "$1" != None ]; }

# The stack keeps the table and the content bucket when it is deleted, so their
# names are read first and they are removed once the stack is gone. If the
# stack cannot be deleted the data is left alone.
destroy() {
    aws cloudformation describe-stacks --stack-name "$STACK" >/dev/null 2>&1 \
        || die "there is no stack '$STACK' in account $ACCOUNT, region $REGION"
    echo "This deletes EVERYTHING in '$STACK' (account $ACCOUNT, region $REGION):"
    echo "the game, the API and the players' data in DynamoDB and S3. It cannot be undone."
    read -r -p "Type the stack name to continue: " answer
    [ "$answer" = "$STACK" ] || die "cancelled"

    local site table content
    site="$(output SiteBucketName)"
    table="$(output TableName)"
    content="$(output ContentBucketName)"
    present "$site" && aws s3 rm "s3://$site" --recursive >/dev/null
    echo "Deleting the stack (CloudFront takes a few minutes)..."
    aws cloudformation delete-stack --stack-name "$STACK"
    aws cloudformation wait stack-delete-complete --stack-name "$STACK"
    present "$table" && aws dynamodb delete-table --table-name "$table" >/dev/null
    present "$content" && aws s3 rb "s3://$content" --force >/dev/null
    aws s3api head-bucket --bucket "$ARTIFACTS" 2>/dev/null \
        && aws s3 rb "s3://$ARTIFACTS" --force >/dev/null
    echo "Everything was deleted."
}

show_outputs() {
    aws cloudformation describe-stacks --stack-name "$STACK" \
        --query 'Stacks[0].Outputs[].[OutputKey,OutputValue]' --output text
}

case "$COMMAND" in
    backend)  deploy_backend; show_outputs ;;
    web)      deploy_web; echo "Play at: $(output SiteUrl)" ;;
    outputs)  show_outputs ;;
    destroy)  destroy ;;
esac
